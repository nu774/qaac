#include <iostream>
#include <ctime>
#include <cstdarg>
#include <clocale>
#include <algorithm>
#include <functional>
#include <regex>
#include "strutil.h"
#include "win32util.h"
#include <shellapi.h>
#include "options.h"
#include "inputfactory.h"
#include "bitstream.h"
#include "sink.h"
#include "wavsink.h"
#include "cafsink.h"
#include "waveoutsink.h"
#include "peaksink.h"
#include "cuesheet.h"
#include "composite.h"
#include "nullsource.h"
#include "soxresampler.h"
#include "soxlpf.h"
#include "normalize.h"
#include "mixer.h"
#include "Quantizer.h"
#include "scaler.h"
#include "limiter.h"
#include "pipedreader.h"
#include "TrimmedSource.h"
#include "chanmap.h"
#include "logging.h"
#include "textfile.h"
#include "expand.h"
#include "compressor.h"
#include "metadata.h"
#include "wicimage.h"
#ifdef REFALAC
#include "alacenc.h"
#endif
#ifdef QAAC
#include <delayimp.h>
#include "AudioCodecX.h"
#include "CoreAudioEncoder.h"
#include "CoreAudioPaddedEncoder.h"
#include "CoreAudioResampler.h"
#endif
#include <ShlObj.h>
#include <crtdbg.h>

#ifdef REFALAC
#define PROGNAME "refalac"
#elif defined QAAC
#define PROGNAME "qaac"
#endif

static volatile bool g_interrupted = false;

static
BOOL WINAPI console_interrupt_handler(DWORD type)
{
    g_interrupted = true;
    return TRUE;
}

inline
std::wstring errormsg(const std::exception &ex)
{
    return strutil::us2w(ex.what());
}

static
void load_metadata_files(Options *opts)
{
    if (opts->chapter_file) {
        try {
            chapters::load_from_file(opts->chapter_file, &opts->chapters,
                                     opts->textcp);
        } catch (const std::exception &e) {
            LOG(L"WARNING: %s\n", errormsg(e).c_str());
        }
    }
    try {
        auto tag = opts->tagopts.find(Tag::kLyrics);
        if (tag != opts->tagopts.end())
            tag->second = load_text_file(tag->second.c_str(), opts->textcp);
    } catch (const std::exception &e) {
        LOG(L"WARNING: %s\n", errormsg(e).c_str());
    }
    for (size_t i = 0; i < opts->artwork_files.size(); ++i) {
        try {
            uint64_t size;
            char *data = win32::load_with_mmap(opts->artwork_files[i].c_str(),
                                               &size);
            std::shared_ptr<char> dataPtr(data, UnmapViewOfFile);
            auto type = mp4v2::impl::itmf::computeBasicType(data, size);
            if (type == mp4v2::impl::itmf::BT_IMPLICIT)
                throw std::runtime_error("Unknown artwork image type");
            std::vector<char> vec(data, data + size);
            if (opts->artwork_size)
                WICConvertArtwork(data, size, opts->artwork_size, &vec);
            opts->artworks.push_back(vec);
        } catch (const std::exception &e) {
            LOG(L"WARNING: %s\n", errormsg(e).c_str());
        }
    }
}

static
std::wstring get_output_filename(const wchar_t *ifilename, const Options &opts)
{
    if (opts.ofilename)
        return !std::wcscmp(opts.ofilename, L"-") ? L"-"
                : win32::GetFullPathNameX(opts.ofilename);

    const wchar_t *ext = opts.extension();
    const wchar_t *outdir = opts.outdir ? opts.outdir : L".";
    if (!std::wcscmp(ifilename, L"-"))
        return std::wstring(L"stdin") + ext;

    std::wstring obasename =
        win32::PathReplaceExtension(PathFindFileNameW(ifilename), ext);
    /*
     * Prefixed pathname starting with \\?\ is required to be canonical
     * full pathname.
     * Since libmp4v2 simply prepends \\?\ if it looks like a full pathname,
     * we have to normalize pathname beforehand (by GetFullPathName()).
     */
    std::wstring ofilename =
        win32::GetFullPathNameX(strutil::format(L"%s/%s", outdir,
                                                obasename.c_str()));

    /* test if ifilename and ofilename refer to the same file */
    std::shared_ptr<FILE> ifp, ofp;
    try {
        ifp = win32::fopen(ifilename, L"rb");
        ofp = win32::fopen(ofilename, L"rb");
    } catch (...) {
        return ofilename;
    }
    BY_HANDLE_FILE_INFORMATION ibhi = { 0 }, obhi = { 0 };
    HANDLE ih = reinterpret_cast<HANDLE>(_get_osfhandle(fileno(ifp.get())));
    HANDLE oh = reinterpret_cast<HANDLE>(_get_osfhandle(fileno(ofp.get())));
    GetFileInformationByHandle(ih, &ibhi);
    GetFileInformationByHandle(oh, &obhi);
    if (ibhi.dwVolumeSerialNumber != obhi.dwVolumeSerialNumber ||
        ibhi.nFileIndexHigh != obhi.nFileIndexHigh ||
        ibhi.nFileIndexLow != obhi.nFileIndexLow)
        return ofilename;

    std::wstring tl = strutil::format(L"_%s", ext);
    return win32::PathReplaceExtension(ofilename, tl.c_str());
}

static
void secondsToHMS(double seconds, int *h, int *m, int *s, int *millis)
{
    *h = seconds / 3600;
    seconds -= *h * 3600;
    *m = seconds / 60;
    seconds -= *m * 60;
    *s = seconds;
    *millis = (seconds - *s) * 1000;
}

static
std::wstring formatSeconds(double seconds)
{
    int h, m, s, millis;
    secondsToHMS(seconds, &h, &m, &s, &millis);
    return h ? strutil::format(L"%d:%02d:%02d.%03d", h, m, s, millis)
             : strutil::format(L"%d:%02d.%03d", m, s, millis);
}

class Timer {
    DWORD m_ticks;
public:
    Timer() { m_ticks = GetTickCount(); };
    double ellapsed() {
        return (static_cast<double>(GetTickCount()) - m_ticks) / 1000.0;
    }
};

class PeriodicDisplay {
    uint32_t m_interval;
    uint32_t m_last_tick_title;
    uint32_t m_last_tick_stderr;
    std::wstring m_message;
    bool m_verbose;
    bool m_console_visible;
public:
    PeriodicDisplay(uint32_t interval, bool verbose=true)
        : m_interval(interval),
          m_verbose(verbose)
    {
        m_console_visible = IsWindowVisible(GetConsoleWindow());
        m_last_tick_title = m_last_tick_stderr = GetTickCount();
    }
    void put(const std::wstring &message) {
        m_message = message;
        uint32_t tick = GetTickCount();
        if (tick - m_last_tick_stderr > m_interval) {
            m_last_tick_stderr = tick;
            flush();
        }
    }
    void flush() {
        if (m_verbose) std::fputws(m_message.c_str(), stderr);
        if (m_console_visible &&
            m_last_tick_stderr - m_last_tick_title > m_interval * 4)
        {
            std::vector<wchar_t> s(m_message.size() + 1);
            std::wcscpy(&s[0], m_message.c_str());
            strutil::squeeze(&s[0], L"\r");
            std::wstring msg = strutil::format(L"%hs %s", PROGNAME, &s[0]);
            SetConsoleTitleW(msg.c_str());
            m_last_tick_title = m_last_tick_stderr;
        }
    }
};

class Progress {
    PeriodicDisplay m_disp;
    bool m_verbose;
    uint64_t m_total;
    uint32_t m_rate;
    std::wstring m_tstamp;
    Timer m_timer;
    bool m_console_visible;
    DWORD m_stderr_type;
    int m_last_percent;
public:
    Progress(bool verbosity, uint64_t total, uint32_t rate)
        : m_disp(100, verbosity), m_verbose(verbosity),
          m_total(total), m_rate(rate), m_last_percent(0)
    {
        long h = _get_osfhandle(_fileno(stderr));
        m_stderr_type = GetFileType(reinterpret_cast<HANDLE>(h));
        m_console_visible = IsWindowVisible(GetConsoleWindow());
        if (total != ~0ULL)
            m_tstamp = formatSeconds(static_cast<double>(total) / rate);
    }
    void update(uint64_t current)
    {
        if ((!m_verbose || !m_stderr_type) && !m_console_visible) return;
        double fcurrent = current;
        double percent = 100.0 * fcurrent / m_total;
        double seconds = fcurrent / m_rate;
        double ellapsed = m_timer.ellapsed();
        double eta = ellapsed * (m_total / fcurrent - 1);
        double speed = ellapsed ? seconds/ellapsed : 0.0;
        if (m_total == ~0ULL)
            m_disp.put(strutil::format(L"\r%s (%.1fx)   ",
                formatSeconds(seconds).c_str(), speed));
        else {
            std::wstring msg =
                strutil::format(L"\r[%.1f%%] %s/%s (%.1fx), ETA %s  ",
                                percent, formatSeconds(seconds).c_str(),
                                m_tstamp.c_str(), speed,
                                formatSeconds(eta).c_str());
            m_disp.put(msg);
        }
    }
    void finish(uint64_t current)
    {
        m_disp.flush();
        if (m_verbose) fputwc('\n', stderr);
        double ellapsed = m_timer.ellapsed();
        LOG(L"%lld/%lld samples processed in %s\n",
            current, m_total, formatSeconds(ellapsed).c_str());
    }
};

static
void do_encode(IEncoder *encoder, const std::wstring &ofilename,
               const std::vector<std::shared_ptr<ISource> > &chain,
               const Options &opts)
{
    typedef std::shared_ptr<std::FILE> file_t;
    file_t statPtr;
    if (opts.save_stat) {
        std::wstring statname =
            win32::PathReplaceExtension(ofilename, L".stat.txt");
        statPtr = win32::fopen(statname, L"w");
    }
    IEncoderStat *stat = dynamic_cast<IEncoderStat*>(encoder);

    std::shared_ptr<ISource> src = chain.back();
    Progress progress(opts.verbose, src->length(),
                      src->getSampleFormat().mSampleRate);
    try {
        FILE *statfp = statPtr.get();
        while (!g_interrupted && encoder->encodeChunk(1)) {
            progress.update(src->getPosition());
            if (statfp && stat->framesWritten())
                std::fwprintf(statfp, L"%g\n", stat->currentBitrate());
        }
        progress.finish(src->getPosition());
    } catch (...) {
        LOG(L"\n");
        throw;
    }
}

static
void getRawFormat(const Options &opts, AudioStreamBasicDescription *result)
{
    int bits;
    unsigned char c_type, c_endian = 'L';
    int itype, iendian;

    if (std::swscanf(opts.raw_format, L"%hc%d%hc",
                    &c_type, &bits, &c_endian) < 2)
        throw std::runtime_error("Invalid --raw-format spec");
    if ((itype = strutil::strindex("USF", toupper(c_type))) == -1)
        throw std::runtime_error("Invalid --raw-format spec");
    if ((iendian = strutil::strindex("LB", toupper(c_endian))) == -1)
        throw std::runtime_error("Invalid --raw-format spec");
    if (bits <= 0)
        throw std::runtime_error("Invalid --raw-format spec");
    if (itype < 2 && bits > 32)
        throw std::runtime_error("Bits per sample too large");
    if (itype == 2 && bits != 32 && bits != 64)
        throw std::runtime_error("Invalid bits per sample");

    uint32_t type_tab[] = {
        0, kAudioFormatFlagIsSignedInteger, kAudioFormatFlagIsFloat
    };
    AudioStreamBasicDescription asbd =
        cautil::buildASBDForPCM(opts.raw_sample_rate, opts.raw_channels,
                                bits, type_tab[itype]);
    if (iendian)
        asbd.mFormatFlags |= kAudioFormatFlagIsBigEndian;
    *result = asbd;
}

static void do_optimize(MP4FileX *file, const std::wstring &dst, bool verbose)
{
    try {
        file->FinishWriteX();
        MP4FileCopy optimizer(file);
        optimizer.start(strutil::w2us(dst).c_str());
        uint64_t total = optimizer.getTotalChunks();
        PeriodicDisplay disp(100, verbose);
        for (uint64_t i = 1; optimizer.copyNextChunk(); ++i) {
            int percent = 100.0 * i / total + .5;
            disp.put(strutil::format(L"\rOptimizing...%d%%",
                                     percent).c_str());
        }
        disp.put(L"\rOptimizing...done\n");
        disp.flush();
    } catch (mp4v2::impl::Exception *e) {
        handle_mp4error(e);
    }
}

static double do_normalize(std::vector<std::shared_ptr<ISource> > &chain,
                           const Options &opts, bool seekable)
{
    std::shared_ptr<ISource> src = chain.back();
    Normalizer *normalizer = new Normalizer(src, seekable);
    chain.push_back(std::shared_ptr<ISource>(normalizer));

    LOG(L"Scanning maximum peak...\n");
    uint64_t n = 0, rc;
    Progress progress(opts.verbose, src->length(),
                      src->getSampleFormat().mSampleRate);
    while (!g_interrupted && (rc = normalizer->process(4096)) > 0) {
        n += rc;
        progress.update(src->getPosition());
    }
    progress.finish(src->getPosition());
    LOG(L"Peak value: %g\n", normalizer->getPeak());
    return normalizer->getPeak();
}

static
std::shared_ptr<FILE> open_config_file(const wchar_t *file)
{
    std::vector<std::wstring> search_paths;
    const wchar_t *home = _wgetenv(L"HOME");
    if (home)
        search_paths.push_back(strutil::format(L"%s\\%s", home, L".qaac"));
    wchar_t path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(0, CSIDL_APPDATA, 0, 0, path)))
        search_paths.push_back(strutil::format(L"%s\\%s", path, L"qaac"));
    search_paths.push_back(win32::get_module_directory());
    for (size_t i = 0; i < search_paths.size(); ++i) {
        try {
            std::wstring pathtry =
                strutil::format(L"%s\\%s", search_paths[i].c_str(), file);
            return win32::fopen(pathtry, L"r");
        } catch (...) {
            if (i == search_paths.size() - 1) throw;
        }
    }
    return 0;
}

static
void matrix_from_preset(const Options &opts,
                        std::vector<std::vector<complex_t> > *result)
{
    std::shared_ptr<FILE> fpPtr;
    if (opts.remix_preset) {
        std::wstring path =
            strutil::format(L"matrix\\%s.txt", opts.remix_preset);
        fpPtr = open_config_file(path.c_str());
    } else
        fpPtr = win32::fopen(opts.remix_file, L"r");
    FILE *fp = fpPtr.get();
    int c;
    std::vector<std::vector<complex_t> > matrix;
    std::vector<complex_t> row;
    while ((c = std::getc(fp)) != EOF) {
        if (c == '\n') {
            if (row.size()) {
                matrix.push_back(row);
                row.clear();
            }
        } else if (std::isspace(c)) {
            while (c != '\n' && std::isspace(c = std::getc(fp)))
                ;
            std::ungetc(c, fp);
        } else if (std::isdigit(c) || c == '-') {
            std::ungetc(c, fp);
            double v;
            if (std::fscanf(fp, "%lf", &v) != 1)
                throw std::runtime_error("invalid matrix preset file");
            c = std::getc(fp);
            if (std::strchr("iIjJ", c))
                row.push_back(complex_t(0.0, v));
            else if (std::strchr("kK", c))
                row.push_back(complex_t(0.0, -v));
            else {
                std::ungetc(c, fp);
                row.push_back(complex_t(v, 0.0));
            }
        } else
            throw std::runtime_error("invalid char in matrix preset file");
    }
    if (row.size())
        matrix.push_back(row);
    result->swap(matrix);
}

static void
mapped_source(std::vector<std::shared_ptr<ISource> > &chain,
              const Options &opts, uint32_t *channel_layout,
              bool threading)
{
    uint32_t nchannels = chain.back()->getSampleFormat().mChannelsPerFrame;
    const std::vector<uint32_t> *channels = chain.back()->getChannels();
    std::vector<uint32_t> work;
    if (channels) {
        if (opts.verbose > 1) {
            LOG(L"Input layout: %hs\n",
                chanmap::getChannelNames(*channels).c_str());
        }
        // reorder to Microsoft (USB) order
        work.assign(channels->begin(), channels->end());
        chanmap::convertFromAppleLayout(&work);
        channels = &work;
        std::vector<uint32_t> mapping;
        chanmap::getMappingToUSBOrder(*channels, &mapping);
        if (!util::is_increasing(mapping.begin(), mapping.end())) {
            std::shared_ptr<ISource>
                mapper(new ChannelMapper(chain.back(), mapping,
                                         chanmap::getChannelMask(work)));
            chain.push_back(mapper);
            channels = chain.back()->getChannels();
        }
    }
    // remix
    if (opts.remix_preset || opts.remix_file) {
        if (!input::factory()->libsoxconvolver.loaded())
            LOG(L"WARNING: mixer requires libsoxconvolver. Mixing disabled\n");
        else {
            std::vector<std::vector<complex_t> > matrix;
            matrix_from_preset(opts, &matrix);
            if (opts.verbose > 1 || opts.logfilename) {
                LOG(L"Matrix mixer: %uch -> %uch\n",
                    static_cast<uint32_t>(matrix[0].size()),
                    static_cast<uint32_t>(matrix.size()));
            }
            std::shared_ptr<ISource>
                mixer(new MatrixMixer(chain.back(),
                                      input::factory()->libsoxconvolver,
                                      matrix, !opts.no_matrix_normalize));
            chain.push_back(mixer);
            channels = 0;
            nchannels = chain.back()->getSampleFormat().mChannelsPerFrame;
        }
    }
    // map with --chanmap option
    if (opts.chanmap.size()) {
        if (opts.chanmap.size() != nchannels)
            throw std::runtime_error(
                    "nchannels of input and --chanmap spec unmatch");
        std::shared_ptr<ISource>
            mapper(new ChannelMapper(chain.back(), opts.chanmap));
        chain.push_back(mapper);
    }
    // retrieve original channel layout, taking --chanmask into account
    if (opts.chanmask > 0 && util::bitcount(opts.chanmask) != nchannels)
        throw std::runtime_error("unmatch number of channels with --chanmask");
    int chanmask = opts.chanmask;
    if (chanmask < 0)
        chanmask = channels ? chanmap::getChannelMask(*channels) : 0;
    if (!chanmask && !opts.isLPCM()) {
        if (opts.verbose >1 || opts.logfilename)
            LOG(L"Using default channel layout.\n");
        chanmask = chanmap::defaultChannelMask(nchannels);
    }
    *channel_layout = chanmask;
    if (chanmask) {
        if (opts.isLPCM() && opts.verbose > 1) {
            std::vector<uint32_t> vec;
            chanmap::getChannels(chanmask, &vec);
            LOG(L"Output layout: %hs\n",
                chanmap::getChannelNames(vec).c_str());
        }
    }
    if (opts.isAAC() || opts.isALAC()) {
        // construct mapped channel layout to AAC/ALAC order
        *channel_layout = chanmap::AACLayoutFromBitmap(chanmask);
        std::vector<uint32_t> aacmap;
        chanmap::getMappingToAAC(chanmask, &aacmap);
        if (aacmap.size() &&
            !util::is_increasing(aacmap.begin(), aacmap.end())) {
            std::shared_ptr<ISource>
                mapper(new ChannelMapper(chain.back(), aacmap));
            chain.push_back(mapper);
        }
        if (opts.verbose > 1) {
            std::vector<uint32_t> vec;
            AudioChannelLayout acl = { 0 };
            acl.mChannelLayoutTag = *channel_layout;
            chanmap::getChannels(&acl, &vec);
            LOG(L"Output layout: %hs\n", chanmap::getChannelNames(vec).c_str());
        }
    }
}

static
bool parse_timespec(const wchar_t *spec, double sample_rate, int64_t *result)
{
    int hh, mm, s, ff, sign = 1;
    wchar_t a, b;
    double ss;
    if (!spec || !*spec)
        return false;
    if (std::swscanf(spec, L"%lld%c%c", result, &a, &b) == 2 && a == L's')
        return true;
    if (spec[0] == L'-') {
        sign = -1;
        ++spec;
    }
    if (std::swscanf(spec, L"%d:%d:%d%c%c", &mm, &s, &ff, &a, &b) == 4 &&
        a == L'f')
        ss = mm * 60 + s + ff / 75.0;
    else if (std::swscanf(spec, L"%d:%d:%lf%c", &hh, &mm, &ss, &a) == 3)
        ss = ss + ((hh * 60.0) + mm) * 60.0;
    else if (std::swscanf(spec, L"%d:%lf%c", &mm, &ss, &a) == 2)
        ss = ss + mm * 60.0;
    else if (std::swscanf(spec, L"%lf%c", &ss, &a) != 1)
        return false;

    *result = sign * static_cast<int64_t>(sample_rate * ss + .5);
    return true;
}

static
std::shared_ptr<ISeekableSource>
select_timeline(std::shared_ptr<ISeekableSource> src, const Options & opts)
{
    const AudioStreamBasicDescription &asbd = src->getSampleFormat();
    double rate = asbd.mSampleRate;
    int64_t start = 0, end = 0, delay = 0;
    if (!opts.start && !opts.end && !opts.delay)
        return src;
    if (opts.start && !parse_timespec(opts.start, rate, &start))
        throw std::runtime_error("Invalid time spec for --start");
    if (opts.end   && !parse_timespec(opts.end, rate, &end))
        throw std::runtime_error("Invalid time spec for --end");
    if (opts.delay && !parse_timespec(opts.delay, rate, &delay))
        throw std::runtime_error("Invalid time spec for --delay");

    std::shared_ptr<CompositeSource> cp(new CompositeSource());
    std::shared_ptr<ISeekableSource> ns(new NullSource(asbd));

    if (start > 0 || end > 0)
        src = std::make_shared<TrimmedSource>(src, start,
                                              end ? std::max(0LL, end - start)
                                                  : ~0ULL);

    if (delay > 0) {
        if (opts.verbose > 1 || opts.logfilename)
            LOG(L"Prepend %lld samples (%g seconds)\n", delay,
                static_cast<double>(delay) / rate);
        cp->addSource(std::make_shared<TrimmedSource>(ns, 0, delay));
        cp->addSource(src);
    } else if (delay < 0) {
        delay *= -1;
        if (opts.verbose > 1 || opts.logfilename)
            LOG(L"Trim %lld samples (%g seconds)\n", delay,
                static_cast<double>(delay) / rate);
        cp->addSource(std::make_shared<TrimmedSource>(src, delay, ~0ULL));
    } else {
        cp->addSource(src);
    }
    return cp;
}

#ifdef QAAC
static
void config_aac_codec(AudioConverterX &converter, const Options &opts);

inline uint32_t bound_quality(uint32_t n)
{
    return std::min(n, static_cast<uint32_t>(kAudioConverterQuality_Max));
}

#endif

std::string pcm_format_str(AudioStreamBasicDescription &asbd)
{
    const char *stype[] = { "int", "float" };
    unsigned itype = !!(asbd.mFormatFlags & kAudioFormatFlagIsFloat);
    return strutil::format("%s%d", stype[itype], asbd.mBitsPerChannel);
}

void build_filter_chain_sub(std::shared_ptr<ISeekableSource> src,
                            std::vector<std::shared_ptr<ISource> > &chain,
                            const Options &opts, uint32_t *channel_layout,
                            AudioStreamBasicDescription *inputDesc,
                            AudioStreamBasicDescription *outputDesc,
                            bool normalize_pass=false)
{
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    bool threading = opts.threading && si.dwNumberOfProcessors > 1;

    AudioStreamBasicDescription sasbd = src->getSampleFormat();
#ifdef QAAC
    std::shared_ptr<AudioCodecX> codec;
    if (opts.isAAC() || opts.isALAC())
        codec.reset(new AudioCodecX(opts.output_format));
#endif
    mapped_source(chain, opts, channel_layout, threading);

    if (opts.isAAC() || opts.isALAC()) {
#ifdef QAAC
        uint32_t clayout = 
            (opts.isAAC() && !opts.is_caf) ?
                chanmap::getChannelLayoutForCodec(*channel_layout)
              : *channel_layout;

        if (!codec->isAvailableOutputChannelLayout(clayout))
            throw std::runtime_error("Channel layout not supported");
#endif
#ifdef REFALAC
        switch (*channel_layout) {
        case kAudioChannelLayoutTag_Mono:
        case kAudioChannelLayoutTag_Stereo:
        case kAudioChannelLayoutTag_AAC_3_0:
        case kAudioChannelLayoutTag_AAC_4_0:
        case kAudioChannelLayoutTag_AAC_5_0:
        case kAudioChannelLayoutTag_AAC_5_1:
        case kAudioChannelLayoutTag_AAC_6_1:
        case kAudioChannelLayoutTag_AAC_7_1:
            break;
        default:
            throw std::runtime_error("Not supported channel layout for ALAC");
        }
#endif
    }
    if (opts.lowpass > 0) {
        if (!input::factory()->libsoxconvolver.loaded())
            LOG(L"WARNING: --lowpass requires libsoxconvolver. LPF disabled\n");
        else {
            if (opts.verbose > 1 || opts.logfilename)
                LOG(L"Applying LPF: %dHz\n", opts.lowpass);
            std::shared_ptr<SoxLowpassFilter>
                f(new SoxLowpassFilter(input::factory()->libsoxconvolver,
                                       chain.back(),
                                       opts.lowpass));
            chain.push_back(f);
        }
    }
    AudioStreamBasicDescription iasbd = chain.back()->getSampleFormat();
    AudioStreamBasicDescription oasbd = { 0 };
    oasbd.mFormatID = opts.output_format;
    oasbd.mChannelsPerFrame = iasbd.mChannelsPerFrame;

    double rate = opts.rate > 0 ? opts.rate : iasbd.mSampleRate;
    if (!opts.isAAC())
        oasbd.mSampleRate = rate;
#ifdef QAAC
    else {
        double closest_rate = codec->getClosestAvailableOutputSampleRate(rate);
        if (opts.rate != 0)
            oasbd.mSampleRate = closest_rate;
        else {
            AudioChannelLayout acl = { 0 };
            acl.mChannelLayoutTag =
                chanmap::getChannelLayoutForCodec(*channel_layout);
            AudioStreamBasicDescription iiasbd = iasbd;
            iiasbd.mSampleRate = closest_rate;
            AudioConverterX converter(iasbd, oasbd);
            converter.setInputChannelLayout(acl);
            converter.setOutputChannelLayout(acl);
            config_aac_codec(converter, opts);
            converter.getOutputStreamDescription(&oasbd);
        }
    }
#endif
    if (oasbd.mSampleRate != iasbd.mSampleRate) {
        if (!opts.native_resampler && input::factory()->libsoxr.loaded()) {
            LOG(L"%gHz -> %gHz\n", iasbd.mSampleRate, oasbd.mSampleRate);
            std::shared_ptr<SoxrResampler>
                resampler(new SoxrResampler(input::factory()->libsoxr,
                                            chain.back(),
                                            oasbd.mSampleRate));
            if (opts.verbose > 1 || opts.logfilename)
                LOG(L"Using libsoxr SRC: %hs\n", resampler->engine());
            chain.push_back(resampler);
        } else {
#ifndef QAAC
            LOG(L"WARNING: --rate requires libsoxr\n");
            oasbd.mSampleRate = iasbd.mSampleRate;
#else
            LOG(L"%gHz -> %gHz\n", iasbd.mSampleRate, oasbd.mSampleRate);

            if (opts.native_resampler_quality >= 0 ||
                opts.native_resampler_complexity > 0 ||
                (!opts.isAAC() && !opts.isALAC()))
            {
                AudioStreamBasicDescription
                    sfmt = chain.back()->getSampleFormat();
                if ((sfmt.mFormatFlags & kAudioFormatFlagIsFloat) &&
                    sfmt.mBitsPerChannel < 32)
                {
                    chain.push_back(std::make_shared<Quantizer>(chain.back(),
                                                            32, false, true));
                }
                int quality = opts.native_resampler_quality;
                uint32_t complexity = opts.native_resampler_complexity;
                if (quality == -1) quality = kAudioConverterQuality_Medium;
                if (!complexity) complexity = 'norm';

                CoreAudioResampler *resampler =
                    new CoreAudioResampler(chain.back(), oasbd.mSampleRate,
                                           bound_quality(quality), complexity);
                chain.push_back(std::shared_ptr<ISource>(resampler));
                if (opts.verbose > 1 || opts.logfilename)
                    LOG(L"Using CoreAudio SRC: complexity %hs quality %u\n",
                        util::fourcc(resampler->getComplexity()).svalue,
                        resampler->getQuality());
            }
            else if (opts.verbose > 1 || opts.logfilename)
                LOG(L"Using CoreAudio codec default SRC\n");
#endif
        }
    }
    for (size_t i = 0; i < opts.drc_params.size(); ++i) {
        const DRCParams &p = opts.drc_params[i];
        if (opts.verbose > 1 || opts.logfilename)
            LOG(L"DRC: Threshold %gdB Ratio %g Knee width %gdB\n"
                L"     Attack %gms Release %gms\n",
                p.m_threshold, p.m_ratio, p.m_knee_width,
                p.m_attack, p.m_release);
        std::shared_ptr<FILE> stat_file;
        if (p.m_stat_file) {
            FILE *fp = win32::wfopenx(p.m_stat_file, L"wb");
            stat_file = std::shared_ptr<FILE>(fp, std::fclose);
        }
        std::shared_ptr<ISource>
            compressor(new Compressor(chain.back(),
                                      p.m_threshold,
                                      p.m_ratio,
                                      p.m_knee_width,
                                      p.m_attack,
                                      p.m_release,
                                      stat_file));
        chain.push_back(compressor);
    }
    if (normalize_pass) {
        do_normalize(chain, opts, src->isSeekable());
        if (src->isSeekable())
            return;
    }

    if (opts.gain) {
        double scale = util::dB_to_scale(opts.gain);
        if (opts.verbose > 1 || opts.logfilename)
            LOG(L"Gain adjustment: %gdB, scale factor %g\n",
                opts.gain, scale);
        std::shared_ptr<ISource> scaler(new Scaler(chain.back(), scale));
        chain.push_back(scaler);
    }
    if (opts.limiter) {
        if (opts.verbose > 1 || opts.logfilename)
            LOG(L"Limiter on\n");
        std::shared_ptr<ISource> limiter(new Limiter(chain.back()));
        chain.push_back(limiter);
    }
    if (opts.bits_per_sample) {
        bool is_float = (opts.bits_per_sample == 32 && !opts.isALAC());
        unsigned sbits = chain.back()->getSampleFormat().mBitsPerChannel;
        bool sflags = chain.back()->getSampleFormat().mFormatFlags;

        if (opts.isAAC())
            LOG(L"WARNING: --bits-per-sample has no effect for AAC\n");
        else if (sbits != opts.bits_per_sample ||
                 !!(sflags & kAudioFormatFlagIsFloat) != is_float) {
            std::shared_ptr<ISource>
                isrc(new Quantizer(chain.back(), opts.bits_per_sample,
                                   opts.no_dither, is_float));
            chain.push_back(isrc);
            if (opts.verbose > 1 || opts.logfilename)
                LOG(L"Convert to %d bit\n", opts.bits_per_sample);
        }
    }
    if (opts.isAAC()) {
        AudioStreamBasicDescription sfmt = chain.back()->getSampleFormat();
        if (!(sfmt.mFormatFlags & kAudioFormatFlagIsFloat) ||
            sfmt.mBitsPerChannel != 32)
            chain.push_back(std::make_shared<Quantizer>(chain.back(), 32,
                                                        false, true));
    }
    if (threading && (opts.isAAC() || opts.isALAC())) {
        PipedReader *reader = new PipedReader(chain.back());
        reader->start();
        chain.push_back(std::shared_ptr<ISource>(reader));
        if (opts.verbose > 1 || opts.logfilename)
            LOG(L"Enable threading\n");
    }
    iasbd = chain.back()->getSampleFormat();
    if (opts.verbose > 1)
        LOG(L"Format: %hs -> %hs\n",
            pcm_format_str(sasbd).c_str(), pcm_format_str(iasbd).c_str());

    if (opts.isLPCM())
        oasbd = iasbd;
    else if (opts.isAAC())
        oasbd.mFramesPerPacket = opts.isSBR() ? 2048 : 1024;
    else if (opts.isALAC())
        oasbd.mFramesPerPacket = 4096;

    if (opts.isALAC()) {
        if (!(iasbd.mFormatFlags & kAudioFormatFlagIsSignedInteger))
            throw std::runtime_error(
                "floating point PCM is not supported for ALAC");

        switch (iasbd.mBitsPerChannel) {
        case 16:
            oasbd.mFormatFlags = 1; break;
        case 20:
            oasbd.mFormatFlags = 2; break;
        case 24:
            oasbd.mFormatFlags = 3; break;
        case 32:
            oasbd.mFormatFlags = 4; break;
        default:
            throw std::runtime_error("Not supported bit depth for ALAC");
        }
    }
    if (inputDesc) *inputDesc = iasbd;
    if (outputDesc) *outputDesc = oasbd;
}

void build_filter_chain(std::shared_ptr<ISeekableSource> src,
                        std::vector<std::shared_ptr<ISource> > &chain,
                        const Options &opts, uint32_t *channel_layout,
                        AudioStreamBasicDescription *inputDesc,
                        AudioStreamBasicDescription *outputDesc)
{
    chain.push_back(src);
    build_filter_chain_sub(src, chain, opts, channel_layout,
                           inputDesc, outputDesc, opts.normalize);
    if (opts.normalize && src->isSeekable()) {
        src->seekTo(0);
        Normalizer *normalizer = dynamic_cast<Normalizer*>(chain.back().get());
        double peak = normalizer->getPeak();
        chain.clear();
        chain.push_back(src);
        if (peak > FLT_MIN)
            chain.push_back(std::make_shared<Scaler>(src, 1.0/peak));
        build_filter_chain_sub(src, chain, opts, channel_layout,
                               inputDesc, outputDesc, false);
    }
}

static
bool accept_tag(const std::string &name)
{
    /*
     * We don't want to copy these tags from the source.
     * XXX: should we use white list instead?
     */
    static std::regex black_list[] = {
        std::regex("accuraterip.*"),
        std::regex("compatiblebrands"), /* XXX: ffmpeg metadata for mp4 */
        std::regex("ctdb.*confidence"),
        std::regex("cuesheet"),
        std::regex("cuetrack.*"),
        std::regex("encodedby"),
        std::regex("encodingapplication"),
        std::regex("itunnorm"),
        std::regex("itunpgap"),
        std::regex("itunsmpb"),
        std::regex("log"),
        std::regex("majorbrand"),      /* XXX: ffmpeg metadata for mp4 */
        std::regex("minorversion"),    /* XXX: ffmpeg metadata for mp4 */
        std::regex("replaygain.*"),
    };
    std::string ss;
    for (const char *s = name.c_str(); *s; ++s)
        if (!std::strchr(" -_", *s))
            ss.push_back(tolower(static_cast<unsigned char>(*s)));
    size_t i = 0, end = util::sizeof_array(black_list);
    for (i = 0; i < end; ++i)
        if (std::regex_match(ss, black_list[i]))
            break;
    return i == end;
}

static
void set_tags(ISource *src, ISink *sink, const Options &opts,
              const std::wstring encoder_config)
{
    ITagStore *tagstore = dynamic_cast<ITagStore*>(sink);
    if (!tagstore)
        return;
    MP4SinkBase *mp4sink = dynamic_cast<MP4SinkBase*>(tagstore);
    ITagParser *parser = dynamic_cast<ITagParser*>(src);
    if (parser) {
        const std::map<std::string, std::string> &tags = parser->getTags();
        std::map<std::string, std::string>::const_iterator ssi;
        for (ssi = tags.begin(); ssi != tags.end(); ++ssi)
            if (accept_tag(ssi->first))
                tagstore->setTag(ssi->first, ssi->second);
        if (mp4sink) {
            const std::vector<chapters::entry_t> *chapters =
                parser->getChapters();
            if (chapters)
                mp4sink->setChapters(*chapters);
        }
    }
    tagstore->setTag("encoding application",
        strutil::w2us(opts.encoder_name + L", " + encoder_config));

    std::map<uint32_t, std::wstring>::const_iterator uwi;
    for (uwi = opts.tagopts.begin(); uwi != opts.tagopts.end(); ++uwi) {
        const char *name = M4A::getTagNameFromFourCC(uwi->first);
        if (name)
            tagstore->setTag(name, strutil::w2us(uwi->second));
    }

    std::map<std::string, std::wstring>::const_iterator swi;
    for (swi = opts.longtags.begin(); swi != opts.longtags.end(); ++swi)
        tagstore->setTag(swi->first, strutil::w2us(swi->second));

    if (mp4sink) {
        for (size_t i = 0; i < opts.artworks.size(); ++i)
            mp4sink->addArtwork(opts.artworks[i]);
    }
}

static
void decode_file(const std::vector<std::shared_ptr<ISource> > &chain,
                 const std::wstring &ofilename, const Options &opts,
                 AudioStreamBasicDescription &oasbd, uint32_t chanmask)
{
    const std::shared_ptr<ISource> src = chain.back();
    const AudioStreamBasicDescription &sf = src->getSampleFormat();

    std::shared_ptr<FILE> fileptr;
    std::shared_ptr<ISink> sink;
    CAFSink *cafsink = 0;

    if (opts.isLPCM()) {
        fileptr = win32::fopen(ofilename, L"wb");
        if (!opts.is_caf) {
            sink = std::make_shared<WaveSink>(fileptr.get(), src->length(),
                                              sf, chanmask);
        } else {
            sink = std::make_shared<CAFSink>(fileptr, oasbd, chanmask,
                                             std::vector<uint8_t>());
            cafsink = dynamic_cast<CAFSink*>(sink.get());
            set_tags(chain[0].get(), cafsink, opts, L"");
            cafsink->beginWrite();
        }
    }
    else if (opts.isWaveOut())
        sink = std::make_shared<WaveOutSink>(sf, chanmask);
    else if (opts.isPeak())
        sink = std::make_shared<PeakSink>(sf);

    Progress progress(opts.verbose, src->length(), sf.mSampleRate);
    uint32_t bpf = sf.mBytesPerFrame;
    std::vector<uint8_t> buffer(4096 * bpf);
    try {
        size_t nread;
        while (!g_interrupted &&
               (nread = src->readSamples(&buffer[0], 4096)) > 0) {
            progress.update(src->getPosition());
            sink->writeSamples(&buffer[0], nread * bpf, nread);
        }
        progress.finish(src->getPosition());
    } catch (const std::exception &e) {
        LOG(L"\nERROR: %s\n", errormsg(e).c_str());
    }

    if (opts.isLPCM()) {
        WaveSink *wavsink = dynamic_cast<WaveSink *>(sink.get());
        if (wavsink)
            wavsink->finishWrite();
        else if (cafsink)
            cafsink->finishWrite(AudioFilePacketTableInfo());
    } else if (opts.isPeak()) {
        PeakSink *p = dynamic_cast<PeakSink *>(sink.get());
        LOG(L"peak: %g (%gdB)\n", p->peak(), util::scale_to_dB(p->peak()));
    }
}

static
void finalize_m4a(MP4SinkBase *sink, IEncoder *encoder,
                   const std::wstring &ofilename, const Options &opts)
{
    IEncoderStat *stat = dynamic_cast<IEncoderStat *>(encoder);
    if (opts.chapter_file) {
        try {
            double duration = stat->samplesRead() /
                encoder->getInputDescription().mSampleRate;
            std::vector<chapters::entry_t> chapters;
            chapters::abs_to_duration(opts.chapters, &chapters, duration);
            sink->setChapters(chapters);
        } catch (const std::runtime_error &e) {
            LOG(L"WARNING: %s\n", errormsg(e).c_str());
        }
    }
    sink->writeTags();
    sink->writeBitrates();
    if (!opts.no_optimize)
        do_optimize(sink->getFile(), ofilename, opts.verbose);
    sink->close();
}

#ifdef QAAC
static
std::shared_ptr<ISink> open_sink(const std::wstring &ofilename,
                                 const Options &opts,
                                 const AudioStreamBasicDescription &asbd,
                                 uint32_t channel_layout,
                                 const std::vector<uint8_t> &cookie)
{
    std::vector<uint8_t> asc;
    if (opts.isAAC())
        cautil::parseMagicCookieAAC(cookie, &asc);

    if (ofilename != L"-") win32::fopen(ofilename, L"w");

    if (opts.is_adts)
        return std::make_shared<ADTSSink>(ofilename, asc);
    else if (opts.is_caf)
        return std::make_shared<CAFSink>(ofilename, asbd, channel_layout,
                                         cookie);
    else if (opts.isALAC())
        return std::make_shared<ALACSink>(ofilename, cookie, !opts.no_optimize);
    else if (opts.isAAC())
        return std::make_shared<MP4Sink>(ofilename, asc, !opts.no_optimize);
    throw std::runtime_error("XXX");
}

static
std::wstring get_encoder_config(AudioConverterX &converter)
{
    std::wstring s;
    AudioStreamBasicDescription asbd;
    converter.getOutputStreamDescription(&asbd);
    UInt32 codec = asbd.mFormatID;
    if (codec == 'aac ')
        s = L"AAC-LC Encoder";
    else if (codec == 'aach')
        s = L"AAC-HE Encoder";
    else
        s = L"Apple Lossless Encoder";
    if (codec != 'aac ' && codec != 'aach')
        return s;
    UInt32 value = converter.getBitRateControlMode();
    const wchar_t * strategies[] = { L"CBR", L"ABR", L"CVBR", L"TVBR" };
    s += strutil::format(L", %s", strategies[value]);
    if (value == kAudioCodecBitRateControlMode_Variable) {
        value = converter.getSoundQualityForVBR();
        s += strutil::format(L" q%d", value);
    } else {
        value = converter.getEncodeBitRate();
        s += strutil::format(L" %gkbps", value / 1000.0);
    }
    value = converter.getCodecQuality();
    s += strutil::format(L", Quality %d", value);
    return s;
}

static
void show_available_codec_setttings(UInt32 fmt)
{
    AudioCodecX codec(fmt);
    std::vector<AudioValueRange> srates;
    codec.getAvailableOutputSampleRates(&srates);
    std::vector<UInt32> tags;
    codec.getAvailableOutputChannelLayoutTags(&tags);

    for (size_t i = 0; i < srates.size(); ++i) {
        if (srates[i].mMinimum == 0) continue;
        for (size_t j = 0; j < tags.size(); ++j) {
            if (tags[j] == 0) continue;
            AudioChannelLayout acl = { 0 };
            acl.mChannelLayoutTag = tags[j];
            std::vector<uint32_t> channels;
            chanmap::getChannels(&acl, &channels);
            std::string name = chanmap::getChannelNames(channels);

            AudioStreamBasicDescription iasbd =
                cautil::buildASBDForPCM(srates[i].mMinimum,
                                        channels.size(),
                                        32, kAudioFormatFlagIsFloat);
            AudioStreamBasicDescription oasbd = { 0 };
            oasbd.mFormatID = fmt;
            oasbd.mSampleRate = iasbd.mSampleRate;
            oasbd.mChannelsPerFrame = iasbd.mChannelsPerFrame;

            AudioConverterX converter(iasbd, oasbd);
            converter.setInputChannelLayout(acl);
            converter.setOutputChannelLayout(acl);
            converter.setBitRateControlMode(
                    kAudioCodecBitRateControlMode_Constant);
            std::vector<AudioValueRange> bits;
            converter.getApplicableEncodeBitRates(&bits);

            std::wprintf(L"%hs %gHz %hs --",
                    fmt == 'aac ' ? "LC" : "HE",
                    srates[i].mMinimum, name.c_str());
            for (size_t k = 0; k < bits.size(); ++k) {
                if (!bits[k].mMinimum) continue;
                int delim = k == 0 ? L' ' : L',';
                std::wprintf(L"%c%d", delim, lrint(bits[k].mMinimum / 1000.0));
            }
            std::putwchar(L'\n');
        }
    }
}

static
void show_available_aac_settings()
{
    show_available_codec_setttings('aac ');
    show_available_codec_setttings('aach');
}

static
std::string GetCoreAudioVersion(HMODULE hDll)
{
    HRSRC hRes = FindResourceExW(hDll,
                                 RT_VERSION,
                                 MAKEINTRESOURCEW(VS_VERSION_INFO),
                                 MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US));
    if (!hRes)
        win32::throw_error("FindResourceExW", GetLastError());
    std::string data;
    {
        DWORD cbres = SizeofResource(hDll, hRes);
        HGLOBAL hMem = LoadResource(hDll, hRes);
        if (hMem) {
            char *pc = static_cast<char*>(LockResource(hMem));
            if (cbres && pc)
                data.assign(pc, cbres);
            FreeResource(hMem);
        }
    }
    // find dwSignature of VS_FIXEDFILEINFO
    size_t pos = data.find("\xbd\x04\xef\xfe");
    if (pos != std::string::npos) {
        VS_FIXEDFILEINFO vfi;
        std::memcpy(&vfi, data.c_str() + pos, sizeof vfi);
        WORD v[4];
        v[0] = HIWORD(vfi.dwFileVersionMS);
        v[1] = LOWORD(vfi.dwFileVersionMS);
        v[2] = HIWORD(vfi.dwFileVersionLS);
        v[3] = LOWORD(vfi.dwFileVersionLS);
        return strutil::format("%u.%u.%u.%u", v[0],v[1],v[2],v[3]);
    }
    return "";
}

static
void setup_aach_codec(HMODULE hDll)
{
    CFPlugInFactoryFunction aachFactory =
        AutoCast<void>(GetProcAddress(hDll,
                "ACMP4AACHighEfficiencyEncoderFactory"));
    if (aachFactory) {
        AudioComponentDescription desc = { 'aenc', 'aach', 0 };
        AudioComponentRegister(&desc,
                CFSTR("MPEG4 High Efficiency AAC Encoder"),
                0, aachFactory);
    }
}

static
FARPROC WINAPI DllImportHook(unsigned notify, PDelayLoadInfo pdli)
{
    win32::throw_error(pdli->szDll, pdli->dwLastError);
    return 0;
}

inline
void throwIfError(HRESULT expr, const char *msg)
{
    if (FAILED(expr))
        win32::throw_error(msg, expr);
}
#define HR(expr) (void)(throwIfError((expr), #expr))

static
void set_dll_directories(int verbose)
{
    SetDllDirectoryW(L"");
    DWORD sz = GetEnvironmentVariableW(L"PATH", 0, 0);
    std::vector<wchar_t> vec(sz);
    sz = GetEnvironmentVariableW(L"PATH", &vec[0], sz);
    std::wstring searchPaths(&vec[0], &vec[sz]);

    try {
        HKEY hKey;
        const wchar_t *subkey =
            L"SOFTWARE\\Apple Inc.\\Apple Application Support";
        HR(RegOpenKeyExW(HKEY_LOCAL_MACHINE, subkey, 0, KEY_READ, &hKey));
        std::shared_ptr<HKEY__> hKeyPtr(hKey, RegCloseKey);
        DWORD size;
        HR(RegQueryValueExW(hKey, L"InstallDir", 0, 0, 0, &size));
        std::vector<wchar_t> vec(size/sizeof(wchar_t));
        HR(RegQueryValueExW(hKey, L"InstallDir", 0, 0,
                    reinterpret_cast<LPBYTE>(&vec[0]), &size));
        searchPaths = strutil::format(L"%s;%s", &vec[0], searchPaths.c_str());
    } catch (const std::exception &) {}
    std::wstring dir = win32::get_module_directory() + L"QTfiles";
#ifdef _WIN64
    dir += L"64";
#endif
    searchPaths = strutil::format(L"%s;%s", dir.c_str(), searchPaths.c_str());
    SetEnvironmentVariableW(L"PATH", searchPaths.c_str());
}

static
void config_aac_codec(AudioConverterX &converter,
                      const Options &opts)
{
    converter.setBitRateControlMode(opts.method);
    double bitrate = opts.bitrate * 1000.0;
    if (bitrate == 0.0)
        bitrate = 1000.0 * 1000.0 * 1000.0;
    else if (bitrate < 8000.0) {
        bitrate /= 1000.0;
        AudioStreamBasicDescription iasbd, oasbd;
        converter.getInputStreamDescription(&iasbd);
        converter.getOutputStreamDescription(&oasbd);
        std::shared_ptr<AudioChannelLayout> layout;
        converter.getOutputChannelLayout(&layout);

        double rate = oasbd.mSampleRate ? oasbd.mSampleRate : iasbd.mSampleRate;
        unsigned nchannels = oasbd.mChannelsPerFrame;
        switch (layout->mChannelLayoutTag) {
        case kAudioChannelLayoutTag_MPEG_5_1_D:
        case kAudioChannelLayoutTag_AAC_6_1:
        case kAudioChannelLayoutTag_MPEG_7_1_B:
            --nchannels;
        }
        bitrate *= nchannels * rate;
    }
    if (opts.method != Options::kTVBR) {
        bitrate = converter.getClosestAvailableBitRate(bitrate);
        converter.setEncodeBitRate(bitrate);
    } else {
        converter.setSoundQualityForVBR(bound_quality(opts.bitrate));
    }
    converter.setCodecQuality(bound_quality((opts.quality + 1) << 5));
}

static
bool insert_pce(uint32_t channel_layout, std::vector<uint8_t> *asc)
{
    if (channel_layout != chanmap::kAudioChannelLayoutTag_AAC_7_1_Rear)
        return false;

    BitStream ibs(asc->data(), asc->size());
    BitStream bs;
    uint32_t obj_type = bs.copy(ibs, 5); 
    uint32_t sf_index = bs.copy(ibs, 4);
    uint32_t channel_config = ibs.get(4);
    bs.put(0, 4);
    bs.copy(ibs, 3);

    bs.put(0, 4); /* element_instance_tag */
    bs.put(obj_type, 2);
    bs.put(sf_index, 4);
    bs.put(2, 4); /* num_front_channel_elements */
    bs.put(1, 4); /* num_side_channel_elements  */
    bs.put(1, 4); /* num_back_channel_elements  */
    bs.put(1, 2); /* num_lfe_channel_elements   */
    bs.put(0, 3); /* num_assoc_data_elements    */
    bs.put(0, 4); /* num_valid_cc_elements      */
    bs.put(0, 3); /* mono_mixdown, stereo_mixdown, matrix_mixdown */

    /* C */
    bs.put(0, 1); /* front_element_is_cpe */
    bs.put(0, 4); /* front_element_tag_select */
    /* L+R */
    bs.put(1, 1); /* front_element_is_cpe */
    bs.put(0, 4); /* front_element_tag_select */
    /* Ls+Rs */
    bs.put(1, 1); /* side_element_is_cpe */
    bs.put(1, 4); /* side_element_tag_select */
    /* Rls+Rrs */
    bs.put(1, 1); /* back_element_is_cpe */
    bs.put(2, 4); /* back_element_tag_select */
    /* LFE */
    bs.put(0, 4); /* lfe_elementtag_select */
    bs.byteAlign();

    size_t len = bs.position() / 8;
    std::vector<uint8_t> result(asc->size() + len);
    std::memcpy(result.data(), bs.data(), len);
    if (asc->size() > 2)
        std::memcpy(&result[len], asc->data() + 2, asc->size() - 2);
    asc->swap(result);
    return true;
}

static
void set_encoding_params(AudioConverterX &converter, ITagStore *store)
{
    UInt32 mode = converter.getBitRateControlMode();
    UInt32 bitrate = converter.getEncodeBitRate();
    AudioStreamBasicDescription asbd;
    converter.getOutputStreamDescription(&asbd);
    UInt32 codec = asbd.mFormatID;
    AudioComponentDescription cd = { 'aenc', codec, 'appl', 0, 0 };
    auto ac = AudioComponentFindNext(nullptr, &cd);
    UInt32 vers = 0;
    AudioComponentGetVersion(ac, &vers);

    char buf[32] = "vers\0\0\0\1acbf\0\0\0\0brat\0\0\0\0cdcv\0\0\0";
    buf[12] = mode >> 24;
    buf[13] = mode >> 16;
    buf[14] = mode >> 8;
    buf[15] = mode;
    buf[20] = bitrate >> 24;
    buf[21] = bitrate >> 16;
    buf[22] = bitrate >> 8;
    buf[23] = bitrate;
    buf[28] = vers >> 24;
    buf[29] = vers >> 16;
    buf[30] = vers >> 8;
    buf[31] = vers;
    store->setTag("Encoding Params", std::string(buf, buf + 32));
}

static
void encode_file(const std::shared_ptr<ISeekableSource> &src,
                 const std::wstring &ofilename, const Options &opts,
                 const std::shared_ptr<FILE> *adts_fp=0)
{
    uint32_t channel_layout;
    AudioStreamBasicDescription iasbd, oasbd;

    std::vector<std::shared_ptr<ISource> > chain;
    build_filter_chain(src, chain, opts, &channel_layout, &iasbd, &oasbd);
    if (opts.isLPCM() || opts.isWaveOut() || opts.isPeak()) {
        decode_file(chain, ofilename, opts, oasbd, channel_layout);
        return;
    }
    AudioConverterX converter(iasbd, oasbd);
    AudioChannelLayout acl = { 0 };
    acl.mChannelLayoutTag = chanmap::getChannelLayoutForCodec(channel_layout);
    converter.setInputChannelLayout(acl);
    converter.setOutputChannelLayout(acl);
    if (opts.isAAC()) config_aac_codec(converter, opts);

    std::wstring encoder_config = get_encoder_config(converter);
    LOG(L"%s\n", encoder_config.c_str());
    std::vector<uint8_t> cookie;
    converter.getCompressionMagicCookie(&cookie);

    std::shared_ptr<CoreAudioEncoder> encoder;
    if (opts.isAAC()) {
        std::vector<uint8_t> asc;
        cautil::parseMagicCookieAAC(cookie, &asc);
        if (insert_pce(channel_layout, &asc))
            cautil::replaceASCInMagicCookie(&cookie, asc);

        if (opts.no_smart_padding)
            encoder = std::make_shared<CoreAudioEncoder>(converter);
        else
            encoder = std::make_shared<CoreAudioPaddedEncoder>(
                                         converter, opts.num_priming);
    } else
        encoder = std::make_shared<CoreAudioEncoder>(converter);

    encoder->setSource(chain.back());
    std::shared_ptr<ISink> sink;
    if (adts_fp && adts_fp->get())
        sink = std::make_shared<ADTSSink>(*adts_fp, cookie);
    else
        sink = open_sink(ofilename, opts, oasbd,
                         channel_layout, cookie);
    encoder->setSink(sink);
    if (opts.isAAC()) {
        MP4Sink *mp4sink = dynamic_cast<MP4Sink*>(sink.get());
        if (mp4sink)
            set_encoding_params(converter, mp4sink);
    }
    set_tags(src.get(), sink.get(), opts, encoder_config);
    CAFSink *cafsink = dynamic_cast<CAFSink*>(sink.get());
    if (cafsink)
        cafsink->beginWrite();

    do_encode(encoder.get(), ofilename, chain, opts);
    LOG(L"Overall bitrate: %gkbps\n", encoder->overallBitrate());

    AudioFilePacketTableInfo pti = { 0 };
    if (opts.isAAC()) {
        pti = encoder->getGaplessInfo();
        MP4Sink *mp4sink = dynamic_cast<MP4Sink*>(sink.get());
        if (mp4sink) {
            mp4sink->setGaplessMode(opts.gapless_mode + 1);
            mp4sink->setGaplessInfo(pti);
        }
    }
    MP4SinkBase *mp4sinkbase = dynamic_cast<MP4SinkBase*>(sink.get());
    if (mp4sinkbase)
        finalize_m4a(mp4sinkbase, encoder.get(), ofilename, opts);
    else if (cafsink)
        cafsink->finishWrite(pti);
}
#endif // QAAC
#ifdef REFALAC

static
void encode_file(const std::shared_ptr<ISeekableSource> &src,
        const std::wstring &ofilename, const Options &opts)
{
    uint32_t channel_layout;
    AudioStreamBasicDescription iasbd, oasbd;
    std::vector<std::shared_ptr<ISource> > chain;
    build_filter_chain(src, chain, opts, &channel_layout, &iasbd, &oasbd);

    if (opts.isLPCM() || opts.isWaveOut() || opts.isPeak()) {
        decode_file(chain, ofilename, opts, oasbd, channel_layout);
        return;
    }
    ALACEncoderX encoder(iasbd);
    encoder.setFastMode(opts.alac_fast);
    std::vector<uint8_t> cookie;
    encoder.getMagicCookie(&cookie);

    std::shared_ptr<ISink> sink;
    if (opts.is_caf)
        sink = std::make_shared<CAFSink>(ofilename, oasbd,
                                         channel_layout, cookie);
    else
        sink = std::make_shared<ALACSink>(ofilename, cookie, !opts.no_optimize);
    encoder.setSource(chain.back());
    encoder.setSink(sink);
    set_tags(src.get(), sink.get(), opts, L"Apple Lossless Encoder");
    CAFSink *cafsink = dynamic_cast<CAFSink*>(sink.get());
    if (cafsink)
        cafsink->beginWrite();

    do_encode(&encoder, ofilename, chain, opts);
    LOG(L"Overall bitrate: %gkbps\n", encoder.overallBitrate());

    MP4SinkBase *mp4sinkbase = dynamic_cast<MP4SinkBase*>(sink.get());
    if (mp4sinkbase)
        finalize_m4a(mp4sinkbase, &encoder, ofilename, opts);
    else if (cafsink)
        cafsink->finishWrite(AudioFilePacketTableInfo());
}
#endif

const char *get_qaac_version();

static
void setup_input_factory(const Options &opts)
{
    input::InputFactory *factory = input::factory();

    factory->libsndfile = LibSndfileModule(L"libsndfile-1.dll");
    factory->libflac = FLACModule(L"libFLAC_dynamic.dll");
    if (!factory->libflac.loaded())
        factory->libflac = FLACModule(L"libFLAC.dll");
    if (!factory->libflac.loaded())
        factory->libflac = FLACModule(L"libFLAC-8.dll");
    factory->libwavpack = WavpackModule(L"wavpackdll.dll");
    if (!factory->libwavpack.loaded())
        factory->libwavpack = WavpackModule(L"libwavpack-1.dll");
    factory->libtak = TakModule(L"tak_deco_lib.dll");
#ifdef _WIN64
    factory->libsoxr = SOXRModule(L"libsoxr64.dll");
    if (!factory->libsoxr.loaded())
#endif
        factory->libsoxr = SOXRModule(L"libsoxr.dll");

#ifdef _WIN64
    factory->libsoxconvolver = SoXConvolverModule(L"libsoxconvolver64.dll");
#else
    factory->libsoxconvolver = SoXConvolverModule(L"libsoxconvolver.dll");
#endif
    factory->avisynth = AvisynthModule(L"avisynth.dll");

    if (opts.is_raw) {
        AudioStreamBasicDescription asbd;
        getRawFormat(opts, &asbd);
        factory->setRawFormat(asbd);
    }
    factory->setIgnoreLength(opts.ignore_length);
}

static
void load_cue_sheet(const wchar_t *ifilename, const Options &opts,
                    playlist::Playlist &tracks)
{
    const wchar_t *base_p = PathFindFileNameW(ifilename);
    std::wstring cuedir =
        (base_p == ifilename ? L"." : std::wstring(ifilename, base_p));
    cuedir = win32::GetFullPathNameX(cuedir);

    std::wstring cuetext = load_text_file(ifilename, opts.textcp);
    std::wstringbuf istream(cuetext);
    CueSheet cue;
    cue.parse(&istream);
    cue.loadTracks(tracks, cuedir, 
                   opts.fname_format ? opts.fname_format
                                     : L"${tracknumber}${title& }${title}");
}

static
void load_track(const wchar_t *ifilename, const Options &opts,
                playlist::Playlist &tracks)
{
    const wchar_t *name = L"stdin";
    if (std::wcscmp(ifilename, L"-"))
        name = PathFindFileNameW(ifilename);
    std::wstring ofilename(name);
    const wchar_t *ext = PathFindExtensionW(name);
    std::wstring title(name, ext);

    auto src(input::factory()->open(ifilename));
    auto parser = dynamic_cast<ITagParser*>(src.get());
    if (parser) {
        auto meta = parser->getTags();
        auto tag = meta.find("title");
        if (tag != meta.end())
            title = strutil::us2w(tag->second);
        if (opts.filename_from_tag) {
            auto fn = playlist::generateFileName(opts.fname_format, meta);
            if (fn.size()) ofilename = fn + L".stub";
        }
    }
    playlist::Track new_track;
    new_track.name = title;
    new_track.source = src;
    new_track.ofilename = ofilename;
    tracks.push_back(new_track);
}

static
void group_tracks_with_formats(const playlist::Playlist &tracks,
                               std::vector<playlist::Playlist> *res)
{
    if (!tracks.size())
        return;
    std::vector<playlist::Playlist> vec;
    playlist::Playlist::const_iterator track = tracks.begin();
    do {
        playlist::Playlist group;
        group.push_back(*track);
        const AudioStreamBasicDescription &fmt
            = track->source->getSampleFormat();
        while (++track != tracks.end()) {
            const AudioStreamBasicDescription &afmt
                = track->source->getSampleFormat();
            if (std::memcmp(&fmt, &afmt, sizeof fmt))
                break;
            else
                group.push_back(*track);
        }
        vec.push_back(group);
    } while (track != tracks.end());
    res->swap(vec);
}

struct ConsoleTitleSaver {
    wchar_t title[1024];
    ConsoleTitleSaver()
    {
        GetConsoleTitleW(title, sizeof(title)/sizeof(wchar_t));
    }
    ~ConsoleTitleSaver()
    {
        SetConsoleTitleW(title);
    }
};

struct COMInitializer {
    COMInitializer()
    {
        CoInitialize(0);
    }
    ~COMInitializer()
    {
        CoUninitialize();
    }
};


#ifdef _MSC_VER
int wmain(int argc, wchar_t **argv)
#else
int wmain1(int argc, wchar_t **argv)
#endif
{
#ifdef _DEBUG
//    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF|_CRTDBG_CHECK_ALWAYS_DF);
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF|_CRTDBG_LEAK_CHECK_DF);
#endif
    Options opts;

    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);
    SetDllDirectoryW(L"");
    std::setlocale(LC_CTYPE, "");
    std::setbuf(stderr, 0);
    _setmode(0, _O_BINARY);
    _setmode(2, _O_U8TEXT);
    _setmaxstdio(2048);

#if 0
    FILE *fp = std::fopen("CON", "r");
    std::getc(fp);
#endif
    int result = 0;
    if (!opts.parse(argc, argv))
        return 1;

    COMInitializer __com__;
    std::unique_ptr<Log> logger(Log::instance());;

    try {
        ConsoleTitleSaver consoleTitle;

        if (opts.verbose && !opts.print_available_formats)
            logger->enable_stderr();
        if (opts.logfilename && !opts.print_available_formats)
            logger->enable_file(opts.logfilename);

        if (opts.nice)
            SetPriorityClass(GetCurrentProcess(), IDLE_PRIORITY_CLASS);

        std::string encoder_name;
        encoder_name = strutil::format(PROGNAME " %s", get_qaac_version());
#ifdef QAAC
        __pfnDliFailureHook2 = DllImportHook;
        set_dll_directories(opts.verbose);
        HMODULE hDll = LoadLibraryW(L"CoreAudioToolbox.dll");
        if (!hDll)
            win32::throw_error("CoreAudioToolbox.dll", GetLastError());
        else {
            std::string ver = GetCoreAudioVersion(hDll);
            encoder_name = strutil::format("%s, CoreAudioToolbox %s",
                    encoder_name.c_str(), ver.c_str());
            setup_aach_codec(hDll);
            FreeLibrary(hDll);
        }
#endif
        opts.encoder_name = strutil::us2w(encoder_name);
        if (!opts.print_available_formats)
            LOG(L"%s\n", opts.encoder_name.c_str());

        std::unique_ptr<input::InputFactory>factory(input::factory());
        setup_input_factory(opts);

        if (opts.check_only) {
            if (factory->libsoxconvolver.loaded())
                LOG(L"libsoxconvolver %hs\n",
                    factory->libsoxconvolver.version());
            if (factory->libsoxr.loaded())
                LOG(L"%hs\n", factory->libsoxr.version());
            if (factory->libsndfile.loaded())
                LOG(L"%hs\n", factory->libsndfile.version_string());
            if (factory->libflac.loaded())
                LOG(L"libFLAC %hs\n", factory->libflac.VERSION_STRING);
            if (factory->libwavpack.loaded())
                LOG(L"wavpackdll %hs\n",
                    factory->libwavpack.GetLibraryVersionString());
            if (factory->libtak.loaded()) {
                TtakInt32 var, comp;
                factory->libtak.GetLibraryVersion(&var, &comp);
                LOG(L"tak_deco_lib %u.%u.%u %hs\n",
                        var >> 16, (var >> 8) & 0xff, var & 0xff,
                        factory->libtak.compatible() ? "compatible"
                                                 : "incompatible");
            }
            return 0;
        }
#ifdef QAAC
        if (opts.print_available_formats) {
            show_available_aac_settings();
            return 0;
        }
#endif
        mp4v2::impl::log.setVerbosity(MP4_LOG_NONE);
        //mp4v2::impl::log.setVerbosity(MP4_LOG_VERBOSE4);

        load_metadata_files(&opts);
        if (opts.tmpdir) {
            std::wstring env(L"TMP=");
            env += win32::GetFullPathNameX(opts.tmpdir);
            _wputenv(env.c_str());
        }

        if (opts.ofilename) {
            std::wstring fullpath = win32::GetFullPathNameX(opts.ofilename);
            const wchar_t *ws = fullpath.c_str();
            if (!std::wcscmp(opts.ofilename, L"-"))
                _setmode(1, _O_BINARY);
            else if (std::wcsstr(ws, L"\\\\.\\pipe\\") == ws) {
                if (opts.isMP4())
                    throw std::runtime_error("MP4 piping is not supported");
                opts.ofilename = L"-";
                int pipe = win32::create_named_pipe(ws);
                _close(1);
                _dup2(pipe, 1);
                _close(pipe);
                _setmode(1, _O_BINARY);
            }
        }

        playlist::Playlist tracks;
        const wchar_t *ifilename = 0;

        if (opts.sort_args) {
            struct Sorter {
                static bool cmp(const wchar_t *a, const wchar_t *b)
                {
                    return std::wcscmp(a, b) < 0;
                }
            };
            std::sort(&argv[0], &argv[argc], Sorter::cmp);
        }
        for (int i = 0; i < argc; ++i) {
            ifilename = argv[i];
            if (strutil::wslower(PathFindExtensionW(ifilename)) == L".cue")
                load_cue_sheet(ifilename, opts, tracks);
            else {
                load_track(ifilename, opts, tracks);
                auto src = tracks.back().source;
                auto parser = dynamic_cast<ITagParser*>(src.get());
                do {
                    if (!parser) break;
                    auto &tags = parser->getTags();
                    auto ti = tags.begin();
                    for (; ti != tags.end(); ++ti)
                        if (strcasecmp(ti->first.c_str(), "cuesheet") == 0)
                            break;
                    if (ti == tags.end())
                        break;
                    try {
                        std::wstringbuf wsb(strutil::us2w(ti->second));
                        CueSheet cue;
                        cue.parse(&wsb);
                        tracks.pop_back();
                        cue.loadTracks(tracks, L"", 
                                   opts.fname_format
                                    ? opts.fname_format
                                    : L"${tracknumber}${title& }${title}",
                                    ifilename);
                    } catch (...) {}
                } while (0);
            }
        }
        SetConsoleCtrlHandler(console_interrupt_handler, TRUE);
        if (!opts.concat) {
            for (size_t i = 0; i < tracks.size() && !g_interrupted; ++i) {
                playlist::Track &track = tracks[i];
                if (opts.cue_tracks.size()) {
                    if (std::find(opts.cue_tracks.begin(),
                                  opts.cue_tracks.end(), track.number)
                        == opts.cue_tracks.end())
                        continue;
                }
                std::wstring ofilename =
                    get_output_filename(track.ofilename.c_str(), opts);
                const wchar_t *ofn = L"<stdout>";
                if (ofilename != L"-")
                    ofn = PathFindFileNameW(ofilename.c_str());
                LOG(L"\n%s\n", ofn);
                std::shared_ptr<ISeekableSource> src =
                    select_timeline(track.source, opts);
                src->seekTo(0);
                encode_file(src, ofilename, opts);
            }
        } else {
            std::wstring ofilename = get_output_filename(ifilename, opts);
            LOG(L"\n%s\n",
                ofilename == L"-" ? L"<stdout>"
                                  : PathFindFileNameW(ofilename.c_str()));
            std::shared_ptr<FILE> adts_fp;
            if (opts.is_adts)
                adts_fp = win32::fopen(ofilename, L"wb+");
            std::vector<playlist::Playlist> groups;
            group_tracks_with_formats(tracks, &groups);
            if (!opts.is_adts && groups.size() > 1)
                throw std::runtime_error("Concatenation of multiple inputs "
                                         "with different sample format is "
                                         "only supported for ADTS output");
            std::vector<playlist::Playlist>::const_iterator group;
            playlist::Playlist::const_iterator track;
            for (group = groups.begin();
                 group != groups.end() && !g_interrupted; ++group) { 
                std::shared_ptr<CompositeSource> cs
                    = std::make_shared<CompositeSource>();
                for (track = group->begin(); track != group->end(); ++track) {
                    if (opts.cue_tracks.size()) {
                        if (std::find(opts.cue_tracks.begin(),
                                      opts.cue_tracks.end(), track->number)
                            == opts.cue_tracks.end())
                            continue;
                    }
                    cs->addSourceWithChapter(track->source, track->name);
                }
                std::shared_ptr<ISeekableSource> src(select_timeline(cs, opts));
                src->seekTo(0);
#ifdef REFALAC
                encode_file(src, ofilename, opts);
#else
                encode_file(src, ofilename, opts, &adts_fp);
#endif
            }
        }
        if (opts.isWaveOut())
            WaveOutDevice::instance()->close();
    } catch (const std::exception &e) {
        if (opts.print_available_formats)
            logger->enable_stderr();
        LOG(L"ERROR: %s\n", errormsg(e).c_str());
        result = 2;
    }
    return result;
}

#ifdef __MINGW32__
int main()
{
    int argc;
    wchar_t **argv, **envp;
    _startupinfo si = { 0 };
    __wgetmainargs(&argc, &argv, &envp, 1, &si);
    return wmain1(argc, argv);
}
#endif
