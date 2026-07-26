#include <chrono>
#include <clocale>
#include <numeric>
#include <regex>
#include <thread>
#ifndef _WIN32
#include <sys/mman.h>
#include <termios.h>
#include <unistd.h>
#ifdef __APPLE__
#include <sys/sysctl.h>
#endif
#endif
#include "ISink.h"
#include "ascutil.h"
#include "platformutil.h"
#include "options.h"
#include "InputFactory.h"
#include "sink.h"
#include "WaveSink.h"
#include "CAFSink.h"
#include "SoundIoOutDevice.h"
#include "PeakSink.h"
#include "MMTISOBMFFAACSink.h"
#include "MMTISOBMFFALACSink.h"
#include "cuesheet.h"
#include "CompositeSource.h"
#include "NullSource.h"
#include "SoxrResampler.h"
#include "SoxLowpassFilter.h"
#include "Normalizer.h"
#include "MatrixMixer.h"
#include "Quantizer.h"
#include "Scaler.h"
#include "Limiter.h"
#include "PipedReader.h"
#include "Subprocess.h"
#include "TrimmedSource.h"
#include "chanmap.h"
#include "ChannelMapper.h"
#include "logging.h"
#include "ilo/logging_backend.h"
#include "Compressor.h"
#include "metadata.h"
#ifdef _WIN32
#include "wicimage.h"
#endif
#include "FLACModule.h"
#include "LibSndfileSource.h"
#ifdef _WIN32
#include "TakSource.h"
#endif
#include "WavpackSource.h"
#ifdef _WIN32
#include "AvisynthSource.h"
#endif
#include "OpusPacketDecoder.h"
#include "VorbisPacketDecoder.h"
#include "OggIndex.h"
#include "OggSource.h"
#include "SeekableInputStream.h"
#ifdef REFALAC
#include "ALACEncoderX.h"
#endif
#ifdef QAAC
#ifdef _WIN32
#include <delayimp.h>
#endif
#include "AudioCodecX.h"
#include "CoreAudioEncoder.h"
#include "CoreAudioPaddedEncoder.h"
#include "CoreAudioResampler.h"
#endif
#include <csignal>
#ifdef _WIN32
#include <crtdbg.h>
#else
#include <cstdlib>
#endif

#ifdef REFALAC
#define PROGNAME "refalac"
#elif defined QAAC
#define PROGNAME "qaac"
#endif

#include "PeriodicDisplay.h"

static volatile sig_atomic_t g_interrupted = 0;

#ifdef _WIN32
static
BOOL WINAPI console_interrupt_handler(DWORD type)
{
    g_interrupted = 1;
    return TRUE;
}
#else
static
void console_interrupt_handler(int sig)
{
    g_interrupted = 1;
}

static
void install_signal_handlers()
{
    struct sigaction sa = { 0 };
    sa.sa_handler = console_interrupt_handler;
    sigemptyset(&sa.sa_mask);
    sigaddset(&sa.sa_mask, SIGINT);
    sigaddset(&sa.sa_mask, SIGTERM);
    sigaddset(&sa.sa_mask, SIGHUP);
    sigaction(SIGINT, &sa, 0);
    sigaction(SIGTERM, &sa, 0);
    sigaction(SIGHUP, &sa, 0);

    struct sigaction ignore = { 0 };
    ignore.sa_handler = SIG_IGN;
    sigemptyset(&ignore.sa_mask);
    sigaction(SIGPIPE, &ignore, 0);
}
#endif

inline
std::string errormsg(const std::exception &ex)
{
    return ex.what();
}

class Progress {
    PeriodicDisplay m_disp;
    bool m_verbose;
    uint64_t m_total;
    uint32_t m_rate;
    std::string m_tstamp;
    platform::Timer m_timer;
    bool m_console_visible;
    bool m_stderr_valid;
    bool m_show_eta;
public:
    Progress(bool verbosity, uint64_t total, uint32_t rate, bool showEta = true)
        : m_disp(100, verbosity), m_verbose(verbosity),
          m_total(total), m_rate(rate), m_show_eta(showEta)
    {
#ifdef _WIN32
        m_stderr_valid =
            GetFileType(platform::get_handle(_fileno(stderr))) != FILE_TYPE_UNKNOWN;
        m_console_visible = IsWindowVisible(GetConsoleWindow());
#else
        m_stderr_valid = true;
        m_console_visible = false; // no title-bar update path on POSIX
#endif
        if (total != ~0ULL)
            m_tstamp = util::format_seconds(static_cast<double>(total) / rate);
    }
    void update(uint64_t current)
    {
        if ((!m_verbose || !m_stderr_valid) && !m_console_visible) return;
        double fcurrent = current;
        double percent = 100.0 * fcurrent / m_total;
        double seconds = fcurrent / m_rate;
        double ellapsed = m_timer.ellapsed();
        double speed = ellapsed ? seconds/ellapsed : 0.0;
        if (m_total == ~0ULL)
            m_disp.put(strutil::format("\r%s (%.1fx)   ",
                util::format_seconds(seconds).c_str(), speed));
        else if (!m_show_eta) {
            std::string msg =
                strutil::format("\r[%.1f%%] %s/%s (%.1fx)  ",
                                percent, util::format_seconds(seconds).c_str(),
                                m_tstamp.c_str(), speed);
            m_disp.put(msg);
        } else {
            double eta = fcurrent > 0 ? ellapsed * (m_total / fcurrent - 1) : 0.0;
            std::string msg =
                strutil::format("\r[%.1f%%] %s/%s (%.1fx), ETA %s  ",
                                percent, util::format_seconds(seconds).c_str(),
                                m_tstamp.c_str(), speed,
                                util::format_seconds(eta).c_str());
            m_disp.put(msg);
        }
    }
    void finish(uint64_t current)
    {
        m_disp.flush();
        if (m_verbose) fputc('\n', stderr);
        double ellapsed = m_timer.ellapsed();
        LOG("%lld/%lld samples processed in %s\n",
            current, m_total, util::format_seconds(ellapsed).c_str());
    }
};

static
ca::AudioStreamBasicDescription get_encoding_ASBD(const ISource *src,
                                                   uint32_t codecid)
{
    ca::AudioStreamBasicDescription iasbd = src->getSampleFormat();
    ca::AudioStreamBasicDescription oasbd = { 0 };

    oasbd.mFormatID = codecid;
    oasbd.mChannelsPerFrame = iasbd.mChannelsPerFrame;
    oasbd.mSampleRate = iasbd.mSampleRate;

    if (codecid == 'aac ')
        oasbd.mFramesPerPacket = 1024;
    else if (codecid == 'aach')
        oasbd.mFramesPerPacket = 2048;
    else if (codecid == 'alac')
        oasbd.mFramesPerPacket = 4096;

    if (codecid == 'alac') {
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
    return oasbd;
}

static
uint32_t get_encoding_channel_layout(ISource *src, const Options &opts,
                                     uint32_t *bitmap)
{
    ca::AudioStreamBasicDescription asbd = src->getSampleFormat();
    const std::vector<uint32_t> *cs = src->getChannels();
    uint32_t chanmask;
    if (cs) chanmask = chanmap::getChannelMask(*cs);
    else chanmask = chanmap::defaultChannelMask(asbd.mChannelsPerFrame);
    uint32_t tag = chanmap::AACLayoutFromBitmap(chanmask);
#ifdef QAAC
    auto codec = std::make_shared<AudioCodecX>(opts.output_format);
    if (tag == kAudioChannelLayoutTag_AAC_7_1_B) {
        if (opts.isALAC())
            throw std::runtime_error("Channel layout not supported");
    } else if (!codec->isAvailableOutputChannelLayout(tag)) {
        throw std::runtime_error("Channel layout not supported");
    }
#endif
#ifdef REFALAC
    if (!ALACEncoderX::isAvailableOutputChannelLayout(tag))
        throw std::runtime_error("Not supported channel layout for ALAC");
#endif
    if (bitmap) *bitmap = chanmask;
    return tag;
}

double target_sample_rate(const Options &opts, ISource *src)
{
    ca::AudioStreamBasicDescription iasbd = src->getSampleFormat();
    double candidate = opts.rate > 0 ? opts.rate : iasbd.mSampleRate;
#ifdef QAAC
    if (!opts.isAAC())
        return candidate;
    else if (opts.rate != 0) {
        auto codec = std::make_shared<AudioCodecX>(opts.output_format);
        return codec->getClosestAvailableOutputSampleRate(candidate);
    } else {
        uint32_t tag = get_encoding_channel_layout(src, opts, nullptr);
        AudioChannelLayout acl = { 0 };
        acl.mChannelLayoutTag = tag;
        AudioStreamBasicDescription oasbd = { 0 };
        oasbd.mFormatID = opts.output_format;
        oasbd.mChannelsPerFrame = iasbd.mChannelsPerFrame;
        AudioConverterXX converter(cautil::toNative(iasbd), oasbd);
        converter.setInputChannelLayout(acl);
        converter.setOutputChannelLayout(acl);
        int32_t quality = (opts.quality + 1) << 5;
        converter.configAACCodec(opts.method, opts.bitrate, quality);
        return converter.getOutputStreamDescription().mSampleRate;
    }
#else
    return candidate;
#endif
}

static
void manipulate_channels(std::vector<std::shared_ptr<ISource> > &chain,
                         const Options &opts)
{
    // normalize to Microsoft channel layout
    {
        const std::vector<uint32_t> *cs = chain.back()->getChannels();
        if (cs) {
            if (opts.verbose > 1) {
                LOG("Input layout: %s\n",
                    chanmap::getChannelNames(*cs).c_str());
            }
            auto map = chanmap::getMappingToUSBOrder(*cs);
            if (!util::is_increasing(map.begin(), map.end()))
            {
                std::shared_ptr<ISource>
                    mapper(new ChannelMapper(chain.back(), map,
                                             chanmap::getChannelMask(*cs)));
                chain.push_back(mapper);
            }
        }
    }
    // remix
    if (opts.remix_preset || opts.remix_file) {
        std::vector<std::vector<misc::complex_t> > matrix;
        if (opts.remix_file)
            matrix = misc::loadRemixerMatrixFromFile(opts.remix_file);
        else
            matrix = misc::loadRemixerMatrixFromPreset(opts.remix_preset);
        if (opts.verbose > 1 || opts.logfilename) {
            LOG("Matrix mixer: %uch -> %uch\n",
                static_cast<uint32_t>(matrix[0].size()),
                static_cast<uint32_t>(matrix.size()));
        }
        std::shared_ptr<ISource>
            mixer(new MatrixMixer(chain.back(),
                                  matrix, !opts.no_matrix_normalize));
        chain.push_back(mixer);
    }

    uint32_t nchannels = chain.back()->getSampleFormat().mChannelsPerFrame;

    // --chanmap
    if (opts.chanmap.size()) {
        if (opts.chanmap.size() != nchannels)
            throw std::runtime_error(
                    "nchannels of input and --chanmap spec unmatch");
        std::shared_ptr<ISource>
            mapper(new ChannelMapper(chain.back(), opts.chanmap));
        chain.push_back(mapper);
    }
    // --chanmask
    if (opts.chanmask > 0)
    {
        if (util::bitcount(opts.chanmask) != nchannels)
            throw std::runtime_error("unmatch --chanmask with input");
        std::vector<uint32_t> map(nchannels);
        std::iota(map.begin(), map.end(), 1);
        std::shared_ptr<ISource>
            mapper(new ChannelMapper(chain.back(), map, opts.chanmask));
        chain.push_back(mapper);
    }
}

std::string pcm_format_str(const ca::AudioStreamBasicDescription &asbd)
{
    const char *stype[] = { "int", "float" };
    unsigned itype = !!(asbd.mFormatFlags & kAudioFormatFlagIsFloat);
    return strutil::format("%s%d", stype[itype], asbd.mBitsPerChannel);
}

static double do_normalize(std::vector<std::shared_ptr<ISource> > &chain,
                           const Options &opts, bool seekable)
{
    std::shared_ptr<ISource> src = chain.back();
    Normalizer *normalizer = new Normalizer(src, seekable);
    chain.push_back(std::shared_ptr<ISource>(normalizer));

    LOG("Scanning maximum peak...\n");
    uint64_t n = 0, rc;
    Progress progress(opts.verbose, src->length(),
                      src->getSampleFormat().mSampleRate);
    while (!g_interrupted && (rc = normalizer->process(4096)) > 0) {
        n += rc;
        progress.update(src->getPosition());
    }
    progress.finish(src->getPosition());
    LOG("Peak: %g (%gdB)\n", normalizer->getPeak(), util::scale_to_dB(normalizer->getPeak()));
	return normalizer->getPeak();
}

void build_filter_chain_sub(std::shared_ptr<ISeekableSource> src,
                            std::vector<std::shared_ptr<ISource> > &chain,
                            const Options &opts, bool normalize_pass=false)
{
    unsigned nprocessors = std::thread::hardware_concurrency();
    bool threading = opts.threading && nprocessors > 1;

    ca::AudioStreamBasicDescription sasbd = src->getSampleFormat();
    manipulate_channels(chain, opts);
    // check if channel layout is available for codec
    if (opts.isAAC() || opts.isALAC())
        get_encoding_channel_layout(chain.back().get(), opts, nullptr);

    if (opts.lowpass > 0) {
        if (opts.verbose > 1 || opts.logfilename)
            LOG("Applying LPF: %dHz\n", opts.lowpass);
        std::shared_ptr<SoxLowpassFilter>
            f(new SoxLowpassFilter(chain.back(), opts.lowpass));
        chain.push_back(f);
    }
    {
        double irate = chain.back()->getSampleFormat().mSampleRate;
        double orate = target_sample_rate(opts, chain.back().get());
        if (orate != irate) {
            if (!opts.native_resampler && SOXRModule::instance().loaded()) {
                LOG("%gHz -> %gHz\n", irate, orate);
                std::shared_ptr<SoxrResampler>
                    resampler(new SoxrResampler(chain.back(), orate));
                if (opts.verbose > 1 || opts.logfilename)
                    LOG("Using libsoxr SRC: %s\n", resampler->engine());
                chain.push_back(resampler);
            } else {
#ifndef QAAC
                LOG("WARNING: --rate requires libsoxr, resampling disabled\n");
#else
                LOG("%gHz -> %gHz\n", irate, orate);
                ca::AudioStreamBasicDescription sf
                    = chain.back()->getSampleFormat();
                if ((sf.mFormatFlags & kAudioFormatFlagIsFloat)
                  && sf.mBitsPerChannel < 32)
                {
                    std::shared_ptr<ISource>
                        f(new Quantizer(chain.back(), 32, false, true));
                    chain.push_back(f);
                }
                uint32_t complexity = opts.native_resampler_complexity;
                int quality = std::min(opts.native_resampler_quality,
                                       (int)kAudioConverterQuality_Max);
                if (quality < 0) quality = 0;
                if (!complexity) complexity = 'bats';

                std::shared_ptr<ISource>
                    resampler(new CoreAudioResampler(chain.back(), orate,
                                                     quality, complexity));
                chain.push_back(resampler);
                if (opts.verbose > 1 || opts.logfilename) {
                    CoreAudioResampler *p =
                        dynamic_cast<CoreAudioResampler*>(chain.back().get());
                    LOG("Using CoreAudio SRC: complexity %s quality %u\n",
                        util::fourcc(p->getComplexity()).svalue,
                        p->getQuality());
                }
#endif
            }
        }
    }
    for (size_t i = 0; i < opts.drc_params.size(); ++i) {
        const DRCParams &p = opts.drc_params[i];
        if (opts.verbose > 1 || opts.logfilename)
            LOG("DRC: Threshold %gdB Ratio %g Knee width %gdB\n"
                "     Attack %gms Release %gms\n",
                p.m_threshold, p.m_ratio, p.m_knee_width,
                p.m_attack, p.m_release);
        std::shared_ptr<FILE> stat_file;
        if (p.m_stat_file) {
            FILE *fp = platform::fopen(p.m_stat_file, "wb");
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
        do_normalize(chain, opts, false);
    }

    if (opts.gain) {
        double scale = util::dB_to_scale(opts.gain);
        if (opts.verbose > 1 || opts.logfilename)
            LOG("Gain adjustment: %gdB, scale factor %g\n",
                opts.gain, scale);
        std::shared_ptr<ISource> scaler(new Scaler(chain.back(), scale));
        chain.push_back(scaler);
    }
    if (opts.limiter) {
        if (opts.verbose > 1 || opts.logfilename)
            LOG("Limiter on\n");
        std::shared_ptr<ISource> limiter(new Limiter(chain.back()));
        chain.push_back(limiter);
    }
    if (opts.bits_per_sample) {
        bool is_float = (opts.bits_per_sample == 32 && !opts.isALAC());
        unsigned sbits = chain.back()->getSampleFormat().mBitsPerChannel;
        bool sflags = chain.back()->getSampleFormat().mFormatFlags;

        if (opts.isAAC())
            LOG("WARNING: --bits-per-sample has no effect for AAC\n");
        else if (sbits != opts.bits_per_sample ||
                 !!(sflags & kAudioFormatFlagIsFloat) != is_float) {
            std::shared_ptr<ISource>
                isrc(new Quantizer(chain.back(), opts.bits_per_sample,
                                   opts.no_dither, is_float));
            chain.push_back(isrc);
            if (opts.verbose > 1 || opts.logfilename)
                LOG("Convert to %d bit\n", opts.bits_per_sample);
        }
    }
    if (opts.isAAC()) {
        ca::AudioStreamBasicDescription sfmt = chain.back()->getSampleFormat();
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
            LOG("Enable threading\n");
    }
    if (opts.verbose > 1) {
        auto asbd = chain.back()->getSampleFormat();
        LOG("Format: %s -> %s\n",
            pcm_format_str(sasbd).c_str(), pcm_format_str(asbd).c_str());
    }
}

void build_filter_chain(std::shared_ptr<ISeekableSource> src,
                        std::vector<std::shared_ptr<ISource> > &chain,
                        const Options &opts)
{
    chain.push_back(src);
    build_filter_chain_sub(src, chain, opts, opts.normalize);
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
              const std::string encoder_config)
{
    IChapterParser* cp = dynamic_cast<IChapterParser*>(src);
    if (cp) {
        auto& chapters = cp->getChapters();
        IChapterWriter* cs = dynamic_cast<IChapterWriter*>(sink);
        if (chapters.size() && cs) {
            cs->setChapters(chapters);
        }
    }
    ITagStore* tagstore = dynamic_cast<ITagStore*>(sink);
    if (!tagstore)
        return;
    IArtworkWriter* artWriter = dynamic_cast<IArtworkWriter*>(tagstore);
    ITagParser *parser = dynamic_cast<ITagParser*>(src);
    if (parser) {
        const std::map<std::string, std::string> &tags = parser->getTags();
        std::map<std::string, std::string>::const_iterator ssi;
        for (ssi = tags.begin(); ssi != tags.end(); ++ssi) {
            if (!strcasecmp(ssi->first.c_str(), "cover art")) {
                if (opts.copy_artwork && !opts.artworks.size()) {
                    std::vector<char> vec(ssi->second.begin(),
                                          ssi->second.end());
#ifdef _WIN32
                    if (opts.artwork_size)
                        WICConvertArtwork(vec.data(), vec.size(),
                                          opts.artwork_size, &vec);
#else
                    if (opts.artwork_size)
                        LOG("WARNING: --artwork-size is not supported on "
                            "this platform; embedding artwork as-is\n");
#endif
                    if (artWriter)
                        artWriter->addArtwork(vec);
                }
            } else if (accept_tag(ssi->first))
                tagstore->setTag(ssi->first, ssi->second);
        }
    }
    tagstore->setTag("encoding application",
        opts.encoder_name + ", " + encoder_config);

    for (auto uwi = opts.tagopts.begin(); uwi != opts.tagopts.end(); ++uwi) {
        const char *name = M4A::getTagNameFromFourCC(uwi->first);
        if (name)
            tagstore->setTag(name, uwi->second);
    }

    for (auto swi = opts.longtags.begin(); swi != opts.longtags.end(); ++swi)
        tagstore->setTag(swi->first, swi->second);

    if (artWriter) {
        for (size_t i = 0; i < opts.artworks.size(); ++i)
            artWriter->addArtwork(opts.artworks[i]);
    }
}

static
std::vector<std::string> expand_exec_template(
        const std::vector<std::string> &tmpl, const std::string &name)
{
    std::vector<std::string> result;
    result.reserve(tmpl.size());
    for (auto &&tok: tmpl) {
        std::string expanded = tok;
        size_t pos = 0;
        while ((pos = expanded.find("{}", pos)) != std::string::npos) {
            expanded.replace(pos, 2, name);
            pos += name.size();
        }
        result.push_back(expanded);
    }
    return result;
}

static
void decode_file(const std::vector<std::shared_ptr<ISource> > &chain,
                 const std::string &ofilename, const Options &opts)
{
    std::shared_ptr<ChildProcess> child;
    std::shared_ptr<ISink> sink;
    uint32_t chanmask = 0;
    CAFSink *cafsink = 0;
    const std::shared_ptr<ISource> src = chain.back();
    const ca::AudioStreamBasicDescription &sf = src->getSampleFormat();
    const std::vector<uint32_t> *channels = src->getChannels();

    if (channels) {
        chanmask = chanmap::getChannelMask(*channels);
        if (opts.verbose > 1) {
            LOG("Output layout: %s\n",
                chanmap::getChannelNames(*channels).c_str());
        }
    }
    if (opts.isLPCM()) {
        std::shared_ptr<FILE> fileptr;
        if (!opts.exec_command.empty()) {
            std::string name = platform::PathReplaceExtension(ofilename, "");
            auto argv = expand_exec_template(opts.exec_command, name);
            child = std::make_shared<ChildProcess>(argv);
            fileptr = child->stdinFile();
        } else {
            fileptr = platform::openFileOrStd(ofilename, "wb");
        }
        if (!opts.is_caf) {
            sink = std::make_shared<WaveSink>(fileptr, src->length(),
                                              sf, chanmask);
        } else {
            sink = std::make_shared<CAFSink>(fileptr, sf, chanmask,
                                             std::vector<uint8_t>());
            cafsink = dynamic_cast<CAFSink*>(sink.get());
            set_tags(chain[0].get(), cafsink, opts, "");
            cafsink->beginWrite();
        }
    } else /* opts.isPeak() */
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
        LOG("\nERROR: %s\n", errormsg(e).c_str());
    }

    if (opts.isLPCM()) {
        WaveSink *wavsink = dynamic_cast<WaveSink *>(sink.get());
        if (wavsink)
            wavsink->finishWrite();
        else if (cafsink)
            cafsink->finishWrite(ca::AudioFilePacketTableInfo());
    } else {
        PeakSink *p = dynamic_cast<PeakSink *>(sink.get());
        LOG("Peak: %g (%gdB)\n", p->peak(), util::scale_to_dB(p->peak()));
    }
    if (child) {
        sink.reset();
        int code = child->wait();
        if (code != 0)
            throw std::runtime_error(
                strutil::format("--exec command exited with status %d",
                                code));
    }
}

static
uint32_t map_to_aac_channels(std::vector<std::shared_ptr<ISource> > &chain,
                             const Options &opts)
{
    uint32_t chanmask;
    uint32_t tag = get_encoding_channel_layout(chain.back().get(), opts,
                                               &chanmask);
    auto map = chanmap::getMappingToAAC(chanmask);
    std::shared_ptr<ISource>
        mapper(new ChannelMapper(chain.back(), map, 0, tag));
    chain.push_back(mapper);

    if (opts.verbose > 1) {
        AudioChannelLayout acl = { 0 };
        acl.mChannelLayoutTag = tag;
        auto vec = chanmap::getChannels(&acl);
        LOG("Output layout: %s\n", chanmap::getChannelNames(vec).c_str());
    }
    return tag;
}

static
void do_encode(IEncoder *encoder, const std::string &ofilename,
               const Options &opts)
{
    typedef std::shared_ptr<std::FILE> file_t;
    file_t statPtr;
    if (opts.save_stat) {
        std::string statname =
            platform::PathReplaceExtension(ofilename, ".stat.txt");
        statPtr = file_t(platform::fopen(statname, "w"), std::fclose);
    }
    IEncoderStat *stat = dynamic_cast<IEncoderStat*>(encoder);

    ISource *src = encoder->src();
    Progress progress(opts.verbose, src->length(),
                      src->getSampleFormat().mSampleRate);
    try {
        FILE *statfp = statPtr.get();
        while (!g_interrupted && encoder->encodeChunk(1)) {
            progress.update(src->getPosition());
            if (statfp && stat->framesWritten())
                platform::fprintf(statfp, "%g\n", stat->currentBitrate());
        }
        progress.finish(src->getPosition());
    } catch (...) {
        LOG("\n");
        throw;
    }
}

static
void applyExternalChapterFile(const std::shared_ptr<ISink> &sink,
                              IEncoder *encoder, const Options &opts)
{
    if (!opts.chapter_file)
        return;
    IChapterWriter *cw = dynamic_cast<IChapterWriter*>(sink.get());
    if (!cw)
        return;
    try {
        IEncoderStat *stat = dynamic_cast<IEncoderStat *>(encoder);
        double duration = stat->samplesRead() /
            encoder->getInputDescription().mSampleRate;
        cw->setChapters(misc::convertChaptersToQT(opts.chapters, duration));
    } catch (const std::runtime_error &e) {
        LOG("WARNING: %s\n", errormsg(e).c_str());
    }
}

static
void finishWriteSink(const std::shared_ptr<ISink> &sink, IEncoder *encoder,
                     const ca::AudioFilePacketTableInfo &pti, const Options &opts)
{
    IBitrateWriter *bw = dynamic_cast<IBitrateWriter*>(sink.get());
    if (bw) {
        IEncoderStat *stat = dynamic_cast<IEncoderStat *>(encoder);
        bw->writeBitrates(stat->overallBitrate() * 1000.0 + .5);
    }
    MP4SinkBase *mp4sink = dynamic_cast<MP4SinkBase*>(sink.get());
    if (mp4sink) {
        PeriodicDisplay disp(100, opts.verbose);
        auto optimize_cb = [disp](uint64_t i, uint64_t total) mutable {
            if (total) {
                int percent = 100.0 * i / total + .5;
                disp.put(strutil::format("\rOptimizing...%d%%", percent));
            } else {
                disp.put("\rOptimizing...done\n");
                disp.flush();
            }
        };
        mp4sink->setOptimizeCallback(optimize_cb);
    }
    IFinishWriteSink *fsink = dynamic_cast<IFinishWriteSink*>(sink.get());
    if (fsink)
        fsink->finishWrite(pti);
}

static
ca::AudioStreamBasicDescription prepare_encode_target(
        std::vector<std::shared_ptr<ISource> > &chain, const Options &opts,
        uint32_t *channel_layout, ca::AudioStreamBasicDescription *iasbd)
{
    *channel_layout = map_to_aac_channels(chain, opts);
    *iasbd = chain.back()->getSampleFormat();
    return get_encoding_ASBD(chain.back().get(), opts.output_format);
}

static
void run_encode(IEncoder *encoder, const std::shared_ptr<ISink> &sink,
               const std::string &ofilename, const Options &opts)
{
    CAFSink *cafsink = dynamic_cast<CAFSink*>(sink.get());
    if (cafsink)
        cafsink->beginWrite();

    do_encode(encoder, ofilename, opts);
    LOG("Overall bitrate: %gkbps\n",
        dynamic_cast<IEncoderStat*>(encoder)->overallBitrate());
}

static
void finish_encode(IEncoder *encoder, const std::shared_ptr<ISink> &sink,
                   const ca::AudioFilePacketTableInfo &pti, const Options &opts)
{
    applyExternalChapterFile(sink, encoder, opts);
    finishWriteSink(sink, encoder, pti, opts);
}

#ifdef QAAC
static
std::shared_ptr<ISink> open_sink(const std::string &ofilename,
                                 const Options &opts,
                                 const ca::AudioStreamBasicDescription &asbd,
                                 uint32_t channel_layout,
                                 const std::vector<uint8_t> &cookie)
{
    std::vector<uint8_t> asc;
    if (opts.isAAC())
        asc = ascutil::parseMagicCookieAAC(cookie);

    platform::MakeSureDirectoryPathExistsX(ofilename);
    if (opts.isMP4()) {
        std::shared_ptr<FILE> _ = platform::openFileOrStd(ofilename, "wb");
    }
    if (opts.is_adts)
        return std::make_shared<ADTSSink>(ofilename, asc, false);
    else if (opts.is_caf)
        return std::make_shared<CAFSink>(ofilename, asbd, channel_layout,
                                         cookie);
    else if (opts.isALAC())
        return std::make_shared<MMTISOBMFFALACSink>(ofilename, cookie);
    else if (opts.isAAC())
        return std::make_shared<MMTISOBMFFAACSink>(ofilename, cookie,
                                                    opts.gapless_mode);
    throw std::runtime_error("XXX");
}

static
void show_available_codec_setttings(UInt32 fmt)
{
    AudioCodecX codec(fmt);
    auto srates = codec.getAvailableOutputSampleRates();
    auto tags = codec.getAvailableOutputChannelLayoutTags();

    for (size_t i = 0; i < srates.size(); ++i) {
        if (srates[i].mMinimum == 0) continue;
        for (size_t j = 0; j < tags.size(); ++j) {
            if (tags[j] == 0) continue;
            AudioChannelLayout acl = { 0 };
            acl.mChannelLayoutTag = tags[j];
            auto channels = chanmap::getChannels(&acl);
            std::string name = chanmap::getChannelNames(channels);

            ca::AudioStreamBasicDescription iasbd =
                ascutil::buildASBDForPCM(srates[i].mMinimum,
                                        channels.size(),
                                        32, kAudioFormatFlagIsFloat);
            AudioStreamBasicDescription oasbd = { 0 };
            oasbd.mFormatID = fmt;
            oasbd.mSampleRate = iasbd.mSampleRate;
            oasbd.mChannelsPerFrame = iasbd.mChannelsPerFrame;

            AudioConverterXX converter(cautil::toNative(iasbd), oasbd);
            converter.setInputChannelLayout(acl);
            converter.setOutputChannelLayout(acl);
            converter.setBitRateControlMode(
                    kAudioCodecBitRateControlMode_Constant);
            auto bits = converter.getApplicableEncodeBitRates();

            platform::fprintf(stdout, "%s %gHz %s --",
                    fmt == 'aac ' ? "LC" : "HE",
                    srates[i].mMinimum, name.c_str());
            for (size_t k = 0; k < bits.size(); ++k) {
                if (!bits[k].mMinimum) continue;
                int delim = k == 0 ? ' ' : ',';
                platform::fprintf(stdout, "%c%d", delim,
                              static_cast<int>(lrint(bits[k].mMinimum / 1000.0)));
            }
            platform::fprintf(stdout, "\n");
        }
    }
}

static
void show_available_aac_settings()
{
    show_available_codec_setttings('aac ');
    show_available_codec_setttings('aach');
}

#ifdef _WIN32
static
void setup_aach_codec(HMODULE hDll)
{
    CFPlugInFactoryFunction aachFactory =
        AutoCast(GetProcAddress(hDll, "ACMP4AACHighEfficiencyEncoderFactory"));
    if (aachFactory) {
        AudioComponentDescription desc = { 'aenc', 'aach', 0 };
        AudioComponentRegister(&desc,
                CFSTR("MPEG4 High Efficiency AAC Encoder"),
                0, aachFactory);
    }
}
#elif defined(__APPLE__)
static
std::string get_macos_version()
{
    char buf[64] = {0};
    size_t size = sizeof buf - 1;
    if (sysctlbyname("kern.osproductversion", buf, &size, nullptr, 0) != 0)
        return "unknown";
    return buf;
}
#endif

/*
static
FARPROC WINAPI DllImportHook(unsigned notify, PDelayLoadInfo pdli)
{
    platform::throw_error(pdli->szDll, pdli->dwLastError);
    return 0;
}
*/

#ifdef _WIN32
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
        const wchar_t *subkey[] = {
            L"SOFTWARE\\Apple Inc.\\Apple Application Support",
            L"SOFTWARE\\Apple Computer, Inc.\\iTunes",
        };
        for (int i = 0; i < 2; ++i) {
            if (!RegOpenKeyExW(HKEY_LOCAL_MACHINE, subkey[i], 0, KEY_READ, &hKey)) {
                std::shared_ptr<HKEY__> hKeyPtr(hKey, RegCloseKey);
                DWORD size;
                if (!RegQueryValueExW(hKey, L"InstallDir", 0, 0, 0, &size)) {
                    std::vector<wchar_t> vec(size/sizeof(wchar_t));
                    RegQueryValueExW(hKey, L"InstallDir", 0, 0,
                            reinterpret_cast<LPBYTE>(&vec[0]), &size);
                    searchPaths = strutil::format(L"%s;%s", &vec[0], searchPaths.c_str());
                }
            }
        }
    } catch (const std::exception &) {}
    std::wstring dir = strutil::us2w(platform::get_module_directory()) + L"QTfiles";
#ifdef _WIN64
    dir += L"64";
#endif
    searchPaths = strutil::format(L"%s;%s", dir.c_str(), searchPaths.c_str());
    SetEnvironmentVariableW(L"PATH", searchPaths.c_str());
}
#endif

static
void encode_file(std::vector<std::shared_ptr<ISource> > &chain,
                 const std::string &ofilename, const Options &opts)
{
    uint32_t channel_layout;
    ca::AudioStreamBasicDescription iasbd;
    ca::AudioStreamBasicDescription oasbd =
        prepare_encode_target(chain, opts, &channel_layout, &iasbd);
    AudioConverterXX converter(cautil::toNative(iasbd), cautil::toNative(oasbd));
    AudioChannelLayout acl = { 0 };
    acl.mChannelLayoutTag = channel_layout;
    converter.setInputChannelLayout(acl);
    converter.setOutputChannelLayout(acl);
    if (opts.isAAC()) {
        int32_t quality = (opts.quality + 1) << 5;
        converter.configAACCodec(opts.method, opts.bitrate, quality);
    }
    std::string encoder_config = converter.getConfigAsString();
    LOG("%s\n", encoder_config.c_str());
    auto cookie = converter.getCompressionMagicCookie();

    std::shared_ptr<CoreAudioEncoder> encoder;
    if (opts.isAAC()) {
        if (opts.no_smart_padding)
            encoder = std::make_shared<CoreAudioEncoder>(converter);
        else
            encoder = std::make_shared<CoreAudioPaddedEncoder>(
                                         converter, opts.num_priming);
    } else
        encoder = std::make_shared<CoreAudioEncoder>(converter);

    encoder->setSource(chain.back());
    std::shared_ptr<ISink> sink =
        open_sink(ofilename, opts, oasbd, channel_layout, cookie);
    encoder->setSink(sink);
    if (opts.isAAC() && !opts.is_caf) {
        ITagStore *tagstore = dynamic_cast<ITagStore*>(sink.get());
        if (tagstore) {
            std::string params = converter.getEncodingParamsTag();
            tagstore->setTag("Encoding Params", params);
        }
    }
    set_tags(chain[0].get(), sink.get(), opts, encoder_config);

    run_encode(encoder.get(), sink, ofilename, opts);

    ca::AudioFilePacketTableInfo pti = { 0 };
    if (opts.isAAC())
        pti = encoder->getGaplessInfo();
    finish_encode(encoder.get(), sink, pti, opts);
}
#endif // QAAC
#ifdef REFALAC

static
void encode_file(std::vector<std::shared_ptr<ISource> > &chain,
        const std::string &ofilename, const Options &opts)
{
    uint32_t channel_layout;
    ca::AudioStreamBasicDescription iasbd;
    ca::AudioStreamBasicDescription oasbd =
        prepare_encode_target(chain, opts, &channel_layout, &iasbd);
    ALACEncoderX encoder(iasbd);
    encoder.setFastMode(opts.alac_fast);
    auto cookie = encoder.getMagicCookie();

    platform::MakeSureDirectoryPathExistsX(ofilename);
    std::shared_ptr<ISink> sink;
    if (opts.is_caf)
        sink = std::make_shared<CAFSink>(ofilename, oasbd,
                                         channel_layout, cookie);
    else
        sink = std::make_shared<MMTISOBMFFALACSink>(ofilename, cookie);
    encoder.setSource(chain.back());
    encoder.setSink(sink);
    set_tags(chain[0].get(), sink.get(), opts, "Apple Lossless Encoder");

    run_encode(&encoder, sink, ofilename, opts);
    finish_encode(&encoder, sink, ca::AudioFilePacketTableInfo(), opts);
}
#endif

static
void process_file(const std::shared_ptr<ISeekableSource> &src,
                  const std::string &ofilename, const Options &opts)
{
    std::vector<std::shared_ptr<ISource> > chain;
    build_filter_chain(src, chain, opts);

    if (opts.isLPCM() || opts.isPeak())
        decode_file(chain, ofilename, opts);
    else
        encode_file(chain, ofilename, opts);
}

enum class PlaybackOutcome { Completed, PrevFile, PrevFileTail, Quit };

enum class PlaybackCommand {
    None, SeekBack, SeekForward, SeekHome, NextTrack, PrevOrRestart, Quit,
    TogglePause
};

static
PlaybackCommand translatePlaybackKey(PlaybackKey key)
{
    switch (key) {
    case PlaybackKey::Left:     return PlaybackCommand::SeekBack;
    case PlaybackKey::Right:    return PlaybackCommand::SeekForward;
    case PlaybackKey::Home:     return PlaybackCommand::SeekHome;
    case PlaybackKey::End:
    case PlaybackKey::PageDown: return PlaybackCommand::NextTrack;
    case PlaybackKey::PageUp:   return PlaybackCommand::PrevOrRestart;
    case PlaybackKey::Quit:     return PlaybackCommand::Quit;
    case PlaybackKey::Pause:    return PlaybackCommand::TogglePause;
    default:                    return PlaybackCommand::None;
    }
}

static
PlaybackOutcome play_file(const std::vector<std::shared_ptr<ISource> > &chain,
                        const std::string &ofilename, const Options &opts,
                        bool hasPrevTrack, bool isLastTrack)
{
    const std::shared_ptr<ISource> src = chain.back();
    const ca::AudioStreamBasicDescription &sf = src->getSampleFormat();
    const std::vector<uint32_t> *channels = src->getChannels();
    uint32_t chanmask = 0;

    if (channels) {
        chanmask = chanmap::getChannelMask(*channels);
        if (opts.verbose > 1) {
            LOG("Output layout: %s\n",
                chanmap::getChannelNames(*channels).c_str());
        }
    }
    if (!chanmask)
        chanmask = chanmap::defaultChannelMask(sf.mChannelsPerFrame);

    SoundIoOutDevice &soundio = SoundIoOutDevice::instance();
    soundio.open(sf, chanmask);
    ISeekableSource *ss = dynamic_cast<ISeekableSource*>(chain.front().get());

    Progress progress(opts.verbose, src->length(), sf.mSampleRate, false);
    uint32_t bpf = sf.mBytesPerFrame;
    std::vector<uint8_t> buffer(4096 * bpf);
    PlaybackOutcome outcome = PlaybackOutcome::Completed;
    bool reachedEnd = false;
    try {
        while (!g_interrupted) {
            int64_t chunkStart = src->getPosition();
            size_t nread = src->readSamples(&buffer[0], 4096);
            if (nread == 0) { reachedEnd = true; break; }
            soundio.pushSamples(&buffer[0], nread * bpf, nread, ofilename,
                               chunkStart);

            std::string audibleTrack;
            if (soundio.checkTrackChange(&audibleTrack))
                LOG("\n%s\n", audibleTrack == "-" ? "<stdout>"
                                                  : strutil::basename(audibleTrack));
            int64_t audiblePosition;
            if (soundio.getAudiblePosition(&audibleTrack, &audiblePosition)
                && audibleTrack == ofilename)
                progress.update(audiblePosition);

            PlaybackKeyEvent event;
            if (ss && soundio.takePendingKeyEvent(&event)) {
                int64_t lookahead = soundio.queuedFramesAtLastKeyEvent() *
                    ss->getSampleFormat().mSampleRate / sf.mSampleRate;
                int64_t pos = (std::max)(ss->getPosition() - lookahead,
                                         (int64_t)0);
                int64_t samples = static_cast<int64_t>(ss->getSampleFormat().mSampleRate);
                bool stopReading = false;
                switch (translatePlaybackKey(event.key)) {
                case PlaybackCommand::SeekHome:
                    ss->seekTo(0);
                    break;
                case PlaybackCommand::NextTrack:
                    stopReading = true;
                    break;
                case PlaybackCommand::SeekBack:
                    if (hasPrevTrack && pos < samples)
                        outcome = PlaybackOutcome::PrevFileTail;
                    else
                        ss->seekTo((std::max)(pos - samples, (int64_t)0));
                    break;
                case PlaybackCommand::SeekForward:
                    ss->seekTo((std::min)(pos + samples, (int64_t)ss->length()));
                    break;
                case PlaybackCommand::PrevOrRestart:
                    if (pos >= ss->length() / 10 ||
                        pos >= ss->getSampleFormat().mSampleRate * 5)
                        ss->seekTo(0);
                    else
                        outcome = PlaybackOutcome::PrevFile;
                    break;
                case PlaybackCommand::Quit:
                    outcome = PlaybackOutcome::Quit;
                    break;
                case PlaybackCommand::TogglePause:
                    LOG(soundio.togglePause() ? "\n[Paused]\n" : "\n[Resumed]\n");
                    break;
                case PlaybackCommand::None:
                    break;
                }
                if (outcome != PlaybackOutcome::Completed || stopReading)
                    break;
            }
        }
        if (reachedEnd) {
            if (isLastTrack) {
                // The decoder is exhausted, but up to ~1s of already-decoded
                // audio can still be sitting in the ring buffer. Poll the
                // actually-audible position until it drains, so the display
                // doesn't reach 100% before playback has truly finished.
                auto deadline = std::chrono::steady_clock::now()
                    + std::chrono::seconds(5);
                std::string audibleTrack;
                int64_t audiblePosition;
                while (!g_interrupted
                       && std::chrono::steady_clock::now() < deadline
                       && soundio.getAudiblePosition(&audibleTrack, &audiblePosition)
                       && audibleTrack == ofilename) {
                    progress.update(audiblePosition);
                    std::this_thread::sleep_for(std::chrono::milliseconds(30));
                }
            }
            progress.update(src->length());
        }
        progress.finish(src->getPosition());
    } catch (const std::exception &e) {
        LOG("\nERROR: %s\n", errormsg(e).c_str());
    }
    return outcome;
}

const char *get_qaac_version();

static
ca::AudioStreamBasicDescription getRawFormat(const Options &opts)
{
    int bits;
    unsigned char c_type, c_endian = 'L';
    int itype, iendian;

    if (std::sscanf(opts.raw_format, "%hc%d%hc",
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
    ca::AudioStreamBasicDescription asbd =
        ascutil::buildASBDForPCM(opts.raw_sample_rate, opts.raw_channels,
                                bits, type_tab[itype]);
    if (iendian)
        asbd.mFormatFlags |= kAudioFormatFlagIsBigEndian;
    return asbd;
}

static
std::shared_ptr<ISeekableSource>
trim_input(std::shared_ptr<ISeekableSource> src, const Options & opts)
{
    const ca::AudioStreamBasicDescription &asbd = src->getSampleFormat();
    double rate = asbd.mSampleRate;
    int64_t start = 0, end = 0, delay = 0;
    if (!opts.start && !opts.end && !opts.delay)
        return src;
    if (opts.start && !util::parse_timespec(opts.start, rate, &start))
        throw std::runtime_error("Invalid time spec for --start");
    if (opts.end   && !util::parse_timespec(opts.end, rate, &end))
        throw std::runtime_error("Invalid time spec for --end");
    if (opts.delay && !util::parse_timespec(opts.delay, rate, &delay))
        throw std::runtime_error("Invalid time spec for --delay");
    if (delay) start = delay * -1;

    if (start < 0) {
        std::shared_ptr<CompositeSource> cp(new CompositeSource());
        auto ns = std::make_shared<NullSource>(asbd);
        cp->addSource(std::make_shared<TrimmedSource>(ns, 0, start * -1));
        if (end > 0)
            cp->addSource(std::make_shared<TrimmedSource>(src, 0, end));
        else
            cp->addSource(src);
        return cp;
    }
    uint64_t duration = end ? end - start : ~0ULL;
    return std::make_shared<TrimmedSource>(src, start, duration);
}

typedef std::pair<std::string, std::shared_ptr<ISeekableSource>> workItem;

struct EncodeJob {
    std::shared_ptr<ISeekableSource> src;
    std::string ofilename;
};

static
void load_cue_tracks(const Options &opts, std::streambuf *sb, bool is_embedded,
                     const std::string &path, std::vector<workItem> &items)
{
    CueSheet cue;
    cue.parse(sb);
    auto tracks = cue.loadTracks(is_embedded, path, opts.cue_tracks);
    for (size_t i = 0; i < tracks.size(); ++i) {
        auto parser = dynamic_cast<ITagParser*>(tracks[i].get());
        const char *spec = opts.fname_format;
        if (!spec) spec = "${tracknumber}${title& }${title}";
        std::string ofname =
            misc::generateFileName(spec, parser->getTags());
        items.push_back(std::make_pair(ofname, tracks[i]));
    }
}

static
bool looksLikeOggOpusFlacOrVorbis(const std::shared_ptr<IInputStream> &stream)
{
    unsigned char buf[64] = { 0 };
    stream->seek(0, SEEK_SET);
    if (stream->read(buf, sizeof buf) < 28 || std::memcmp(buf, "OggS", 4))
        return false;
    unsigned nsegs = buf[26];
    size_t packetOffset = 27 + nsegs;
    if (packetOffset + 8 > sizeof buf)
        return false; // unusually large page header; not worth special-casing
    return !std::memcmp(buf + packetOffset, "OpusHead", 8)
        || (buf[packetOffset] == 0x7f
            && !std::memcmp(buf + packetOffset + 1, "FLAC", 4))
        || (buf[packetOffset] == 0x01
            && !std::memcmp(buf + packetOffset + 1, "vorbis", 6));
}

static
void load_ogg_tracks(const char *ifilename, const Options &opts,
                     std::shared_ptr<IInputStream> stream,
                     std::vector<workItem> &tracks)
{
    auto index = std::make_shared<std::vector<OggChainInfo>>();
    OggIndex ix;
    ix.build(stream);
    *index = ix.chains();

    std::string basename(ifilename);
    const char *ext = strutil::file_extension(basename);
    std::string stem(basename.c_str(), ext);

    for (size_t i = 0; i < index->size(); ++i) {
        const OggChainInfo &chain = (*index)[i];
        if (chain.codec != "opus" && chain.codec != "flac"
                && chain.codec != "vorbis") {
            LOG("WARNING: %s: chain %d has unsupported codec (%s), skipped\n",
                strutil::basename(basename), (int)i + 1, chain.codec.c_str());
            continue;
        }
        auto src = std::make_shared<OggSource>(stream, index, i);

        std::string name;
        if (index->size() > 1) {
            const char *spec = opts.fname_format;
            if (!spec) spec = "${tracknumber}${title& }${title}";
            auto fn = misc::generateFileName(spec, src->getTags());
            if (fn.size()) name = fn + ".stub";
            else name = strutil::format("%s-%d%s", stem.c_str(), (int)i + 1, ext);
        } else if (opts.filename_from_tag && opts.fname_format) {
            auto fn = misc::generateFileName(opts.fname_format, src->getTags());
            if (fn.size()) name = fn + ".stub";
        }
        if (name.empty())
            name = basename;
        tracks.push_back(std::make_pair(name, src));
    }
}

static
void load_track(const char *ifilename, const Options &opts,
                std::vector<workItem> &tracks)
{
    std::string ifname(ifilename);
    if (strutil::slower(strutil::file_extension(ifname)) == ".cue") {
        const char *base_p = strutil::basename(ifname);
        std::string cuedir =
            (base_p == ifname.c_str() ? "." : std::string(ifname.c_str(), base_p));
        cuedir = platform::GetFullPathNameX(cuedir);
        std::string cuetext = misc::loadTextFile(ifname, opts.textcp);
        std::stringbuf istream(cuetext);
        load_cue_tracks(opts, &istream, false, cuedir, tracks);
        return;
    }

    auto oggStream = std::make_shared<SeekableInputStream>(ifilename);
    if (looksLikeOggOpusFlacOrVorbis(oggStream)) {
        load_ogg_tracks(ifilename, opts, oggStream, tracks);
        return;
    }

    std::string ofilename(ifilename);
    auto src = InputFactory::instance().open(ifilename);
    auto parser = dynamic_cast<ITagParser*>(src.get());
    if (parser) {
        auto meta = parser->getTags();
        auto cue = meta.find("CUESHEET");
        if (cue != meta.end()) {
            try {
                std::stringbuf wsb(cue->second);
                load_cue_tracks(opts, &wsb, true, ifilename, tracks);
                return;
            } catch (...) {}
        }
        if (opts.filename_from_tag && opts.fname_format) {
            auto fn = misc::generateFileName(opts.fname_format, meta);
            if (fn.size()) ofilename = fn + ".stub";
        }
    }
    tracks.push_back(std::make_pair(ofilename, src));
}

static
void load_metadata_files(Options *opts)
{
    if (opts->chapter_file) {
        try {
            opts->chapters = misc::loadChapterFile(opts->chapter_file,
                                                   opts->textcp);
        } catch (const std::exception &e) {
            LOG("WARNING: %s\n", errormsg(e).c_str());
        }
    }
    for (auto it = opts->ftagopts.begin(); it != opts->ftagopts.end(); ++it) {
        try {
            opts->tagopts[it->first] =
                misc::loadTextFile(it->second, opts->textcp);
        } catch (const std::exception &e) {
            LOG("WARNING: %s\n", errormsg(e).c_str());
        }
    }
    for (size_t i = 0; i < opts->artwork_files.size(); ++i) {
        try {
            uint64_t size;
            char *data = platform::load_with_mmap(opts->artwork_files[i],
                                               &size);
#ifdef _WIN32
            std::shared_ptr<char> dataPtr(data, UnmapViewOfFile);
#else
            std::shared_ptr<char> dataPtr(data,
                [size](char *p) { munmap(p, size); });
#endif
            auto type = mp4v2::impl::itmf::computeBasicType(data, size);
            if (type == mp4v2::impl::itmf::BT_IMPLICIT)
                throw std::runtime_error("Unknown artwork image type");
            std::vector<char> vec(data, data + size);
#ifdef _WIN32
            if (opts->artwork_size)
                WICConvertArtwork(data, size, opts->artwork_size, &vec);
#else
            if (opts->artwork_size)
                LOG("WARNING: --artwork-size is not supported on this "
                    "platform; embedding artwork as-is\n");
#endif
            opts->artworks.push_back(vec);
        } catch (const std::exception &e) {
            LOG("WARNING: %s\n", errormsg(e).c_str());
        }
    }
}

static
std::string get_output_filename(const std::string &ifilename,
                                const Options &opts)
{
    if (opts.ofilename) return opts.ofilename;

    const char *ext = opts.extension();
    const char *outdir = opts.outdir ? opts.outdir : ".";
    if (ifilename == "-")
        return std::string("stdin") + ext;

    std::string obasename =
        platform::PathReplaceExtension(strutil::basename(ifilename), ext);
    std::string ofilename = strutil::format("%s/%s", outdir,
                                             obasename.c_str());

    /* test if ifilename and ofilename refer to the same file */
    std::shared_ptr<FILE> ifp, ofp;
    try {
        ifp = std::shared_ptr<FILE>(platform::fopen(ifilename, "rb"), std::fclose);
        ofp = std::shared_ptr<FILE>(platform::fopen(ofilename, "rb"), std::fclose);
    } catch (...) {
        return ofilename;
    }
    if (!platform::is_same_file(fileno(ifp.get()), fileno(ofp.get())))
        return ofilename;

    std::string tl = strutil::format("_%s", ext);
    return platform::PathReplaceExtension(ofilename, tl.c_str());
}

#ifdef _WIN32
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
#else
struct ConsoleTitleSaver {};
struct COMInitializer {};
#endif

#ifdef _WIN32
struct TtyRawModeGuard {};
#else
struct TtyRawModeGuard {
    struct termios m_orig;
    bool m_active = false;
    TtyRawModeGuard()
    {
        if (tcgetattr(STDIN_FILENO, &m_orig) != 0)
            return; // stdin isn't a real tty (e.g. redirected): leave it alone
        struct termios raw = m_orig;
        // cbreak, not full raw: leave ISIG on so Ctrl-C still delivers
        // SIGINT through the normal path (see install_signal_handlers()).
        raw.c_lflag &= ~(ICANON | ECHO);
        raw.c_cc[VMIN] = 1;
        raw.c_cc[VTIME] = 0;
        if (tcsetattr(STDIN_FILENO, TCSANOW, &raw) == 0)
            m_active = true;
    }
    ~TtyRawModeGuard()
    {
        if (m_active)
            tcsetattr(STDIN_FILENO, TCSANOW, &m_orig);
    }
};
#endif

static
void play_jobs(std::vector<EncodeJob> &jobs, const Options &opts)
{
    TtyRawModeGuard ttyRawMode;
    int64_t pendingSeekPos = -1;
    bool quit = false;
    for (ssize_t i = 0; i < (ssize_t)jobs.size() && !g_interrupted; ++i) {
        const EncodeJob &job = jobs[i];
        job.src->seekTo(pendingSeekPos >= 0 ? pendingSeekPos : 0);
        pendingSeekPos = -1;
        std::vector<std::shared_ptr<ISource> > chain;
        build_filter_chain(job.src, chain, opts);
        bool hasPrevTrack = i > 0;
        bool isLastTrack = i == (ssize_t)jobs.size() - 1;
        PlaybackOutcome outcome = play_file(chain, job.ofilename, opts,
                                           hasPrevTrack, isLastTrack);
        if (outcome == PlaybackOutcome::PrevFile)
            i = std::max(i - 2, (ssize_t)-1);
        else if (outcome == PlaybackOutcome::PrevFileTail) {
            ssize_t target = i - 1;
            const std::shared_ptr<ISeekableSource> &prevSrc = jobs[target].src;
            int64_t step =
                static_cast<int64_t>(prevSrc->getSampleFormat().mSampleRate);
            pendingSeekPos = (std::max)(
                static_cast<int64_t>(prevSrc->length()) - 2 * step, (int64_t)0);
            i = target - 1;
        } else if (outcome == PlaybackOutcome::Quit) {
            quit = true;
            break;
        }
    }
    if (!quit && !g_interrupted)
        SoundIoOutDevice::instance().drain();
}

static int app_main(int argc, char **argv)
{
    Options opts;

    ilo::impl::CLogger::instance().disable_logging();

#ifdef _WIN32
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);
    SetDllDirectoryW(L"");
#endif
    std::setlocale(LC_CTYPE, "");
    std::setbuf(stderr, 0);
#ifdef _WIN32
    _setmode(0, _O_BINARY);
    _setmaxstdio(2048);
#endif

    int result = 0;
    if (!opts.parse(argc, argv))
        return 1;

    COMInitializer __com__;
    Log &logger = Log::instance();

    try {
        ConsoleTitleSaver consoleTitle;

        if (opts.verbose)
            logger.enable_stderr();
        if (opts.logfilename)
            logger.enable_file(opts.logfilename);

#ifdef _WIN32
        if (opts.nice)
            SetPriorityClass(GetCurrentProcess(), IDLE_PRIORITY_CLASS);
#else
        if (opts.nice)
            nice(10);
#endif

        std::string encoder_name;
        encoder_name = strutil::format(PROGNAME " %s", get_qaac_version());
#ifdef QAAC
#ifdef _WIN32
//      decltype(__pfnDliNotifyHook2) __pfnDliFailureHook2 = DllImportHook;
        set_dll_directories(opts.verbose);
        HMODULE hDll = LoadLibraryW(L"CoreAudioToolbox.dll");
        if (!hDll)
            platform::throw_error("CoreAudioToolbox.dll", GetLastError());
        else {
            WORD langid_us = MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US);
            std::string ver = platform::get_dll_version_for_locale(hDll, langid_us);
            encoder_name = strutil::format("%s, CoreAudioToolbox %s",
                                           encoder_name.c_str(), ver.c_str());
            setup_aach_codec(hDll);
            FreeLibrary(hDll);
        }
#else
        encoder_name = strutil::format("%s, macOS AudioToolbox %s",
                                       encoder_name.c_str(),
                                       get_macos_version().c_str());
#endif
#endif
        opts.encoder_name = encoder_name;
        if (!opts.print_available_formats)
            LOG("%s\n", opts.encoder_name.c_str());

        if (opts.check_only) {
            if (SOXRModule::instance().loaded())
                LOG("%s\n", SOXRModule::instance().version());
            if (LibSndfileModule::instance().loaded())
                LOG("%s\n", LibSndfileModule::instance().version_string());
            if (FLACModule::instance().loaded())
                LOG("libFLAC %s\n", FLACModule::instance().VERSION_STRING);
            if (WavpackModule::instance().loaded())
                LOG("wavpackdll %s\n",
                    WavpackModule::instance().GetLibraryVersionString());
#ifdef _WIN32
            if (TakModule::instance().loaded()) {
                TtakInt32 var, comp;
                TakModule::instance().GetLibraryVersion(&var, &comp);
                LOG("tak_deco_lib %u.%u.%u %s\n",
                        var >> 16, (var >> 8) & 0xff, var & 0xff,
                        TakModule::instance().compatible() ? "compatible"
                                                          : "incompatible");
            }
#endif
            if (LibOpusModule::instance().loaded())
                LOG("%s\n", LibOpusModule::instance().get_version_string());
            if (VorbisModule::instance().loaded())
                LOG("%s\n", VorbisModule::instance().version_string());
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
#ifdef _WIN32
            // env var write is a genuine CRT/Win32 boundary
            std::wstring env(L"TMP=");
            env += strutil::us2w(opts.tmpdir);
            _wputenv(env.c_str());
#else
            setenv("TMPDIR", opts.tmpdir, 1);
#endif
        }

#ifdef _WIN32
        if (opts.ofilename && !std::strcmp(opts.ofilename, "-"))
            _setmode(1, _O_BINARY);
#endif

        if (opts.sort_args) {
            std::sort(&argv[0], &argv[argc],
                      [](const char *a, const char *b) {
                          return std::strcmp(a, b) < 0;
                      });
        }
#ifdef _WIN32
        SetConsoleCtrlHandler(console_interrupt_handler, TRUE);
#else
        install_signal_handlers();
#endif

        if (opts.is_raw) {
            InputFactory::instance().setRawFormat(getRawFormat(opts));
        }
        InputFactory::instance().setIgnoreLength(opts.ignore_length);

        struct CleanupScope {
            ~CleanupScope() {
                InputFactory::instance().close();
                SoundIoOutDevice::instance().close();
            }
        } __cleanup__;

        std::vector<workItem> workItems;
        for (int i = 0; i < argc; ++i)
            load_track(argv[i], opts, workItems);

        std::vector<EncodeJob> jobs;
        if (!opts.concat) {
            jobs.reserve(workItems.size());
            for (size_t i = 0; i < workItems.size(); ++i)
                jobs.push_back({ trim_input(workItems[i].second, opts),
                                 get_output_filename(workItems[i].first, opts) });
        } else {
            auto cs = std::make_shared<CompositeSource>();
            for (size_t i = 0; i < workItems.size(); ++i)
                cs->addSourceWithChapter(workItems[i].second, "");
            jobs.push_back({ trim_input(cs, opts),
                             get_output_filename(argv[0], opts) });
        }

        if (opts.isWaveOut()) {
            play_jobs(jobs, opts);
        } else {
            for (ssize_t i = 0; i < (ssize_t)jobs.size() && !g_interrupted; ++i) {
                const EncodeJob &job = jobs[i];
                LOG("\n%s\n",
                    job.ofilename == "-" ? "<stdout>"
                                         : strutil::basename(job.ofilename));
                job.src->seekTo(0);
                process_file(job.src, job.ofilename, opts);
            }
        }
    } catch (const std::exception &e) {
        LOG("ERROR: %s\n", errormsg(e).c_str());
        result = 2;
    }
    return result;
}

#ifdef _WIN32
int wmain(int argc, wchar_t **wargv)
{
    auto cmdline = std::wstring(GetCommandLineW()) + L"\n";
    OutputDebugStringW(cmdline.c_str());

#ifdef _DEBUG
//    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF|_CRTDBG_CHECK_ALWAYS_DF);
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF|_CRTDBG_LEAK_CHECK_DF);
#endif

    std::vector<std::string> uargs(argc);
    std::vector<char*> uargv(argc + 1);
    for (int i = 0; i < argc; ++i) {
        uargs[i] = strutil::w2us(wargv[i]);
        uargv[i] = &uargs[i][0];
    }
    uargv[argc] = 0;
    char **argv = &uargv[0];

    return app_main(argc, argv);
}
#else
int main(int argc, char **argv)
{
    return app_main(argc, argv);
}
#endif
