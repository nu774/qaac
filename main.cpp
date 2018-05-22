#include <clocale>
#include <numeric>
#include <regex>
#include "win32util.h"
#include "options.h"
#include "InputFactory.h"
#include "sink.h"
#include "WaveSink.h"
#include "CAFSink.h"
#include "WaveOutSink.h"
#include "PeakSink.h"
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
#include "TrimmedSource.h"
#include "chanmap.h"
#include "ChannelMapper.h"
#include "logging.h"
#include "Compressor.h"
#include "metadata.h"
#include "wicimage.h"
#include "FLACModule.h"
#include "LibSndfileSource.h"
#include "TakSource.h"
#include "WavpackSource.h"
#include "AvisynthSource.h"
#ifdef REFALAC
#include "ALACEncoderX.h"
#endif
#ifdef QAAC
#include <delayimp.h>
#include "AudioCodecX.h"
#include "CoreAudioEncoder.h"
#include "CoreAudioPaddedEncoder.h"
#include "CoreAudioResampler.h"
#endif
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
        if (m_verbose && m_console_visible &&
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
    win32::Timer m_timer;
    bool m_console_visible;
    DWORD m_stderr_type;
public:
    Progress(bool verbosity, uint64_t total, uint32_t rate)
        : m_disp(100, verbosity), m_verbose(verbosity),
          m_total(total), m_rate(rate)
    {
        m_stderr_type = GetFileType(win32::get_handle(_fileno(stderr)));
        m_console_visible = IsWindowVisible(GetConsoleWindow());
        if (total != ~0ULL)
            m_tstamp = util::format_seconds(static_cast<double>(total) / rate);
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
                util::format_seconds(seconds).c_str(), speed));
        else {
            std::wstring msg =
                strutil::format(L"\r[%.1f%%] %s/%s (%.1fx), ETA %s  ",
                                percent, util::format_seconds(seconds).c_str(),
                                m_tstamp.c_str(), speed,
                                util::format_seconds(eta).c_str());
            m_disp.put(msg);
        }
    }
    void finish(uint64_t current)
    {
        m_disp.flush();
        if (m_verbose) fputwc('\n', stderr);
        double ellapsed = m_timer.ellapsed();
        LOG(L"%lld/%lld samples processed in %s\n",
            current, m_total, util::format_seconds(ellapsed).c_str());
    }
};

static
AudioStreamBasicDescription get_encoding_ASBD(const ISource *src,
                                              uint32_t codecid)
{
    AudioStreamBasicDescription iasbd = src->getSampleFormat();
    AudioStreamBasicDescription oasbd = { 0 };

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
    AudioStreamBasicDescription asbd = src->getSampleFormat();
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
    AudioStreamBasicDescription iasbd = src->getSampleFormat();
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
        AudioConverterXX converter(iasbd, oasbd);
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
                LOG(L"Input layout: %hs\n",
                    chanmap::getChannelNames(*cs).c_str());
            }
            auto ccs = chanmap::convertFromAppleLayout(*cs);
            auto map = chanmap::getMappingToUSBOrder(ccs);
            if (ccs != *cs || !util::is_increasing(map.begin(), map.end()))
            {
                std::shared_ptr<ISource>
                    mapper(new ChannelMapper(chain.back(), map,
                                             chanmap::getChannelMask(ccs)));
                chain.push_back(mapper);
            }
        }
    }
    // remix
    if (opts.remix_preset || opts.remix_file) {
        if (!SoXConvolverModule::instance().loaded())
            LOG(L"WARNING: mixer requires libsoxconvolver. Mixing disabled\n");
        else {
            std::vector<std::vector<misc::complex_t> > matrix;
            if (opts.remix_file)
                matrix = misc::loadRemixerMatrixFromFile(opts.remix_file);
            else
                matrix = misc::loadRemixerMatrixFromPreset(opts.remix_preset);
            if (opts.verbose > 1 || opts.logfilename) {
                LOG(L"Matrix mixer: %uch -> %uch\n",
                    static_cast<uint32_t>(matrix[0].size()),
                    static_cast<uint32_t>(matrix.size()));
            }
            std::shared_ptr<ISource>
                mixer(new MatrixMixer(chain.back(),
                                      matrix, !opts.no_matrix_normalize));
            chain.push_back(mixer);
        }
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

std::string pcm_format_str(AudioStreamBasicDescription &asbd)
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

void build_filter_chain_sub(std::shared_ptr<ISeekableSource> src,
                            std::vector<std::shared_ptr<ISource> > &chain,
                            const Options &opts, bool normalize_pass=false)
{
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    bool threading = opts.threading && si.dwNumberOfProcessors > 1;

    AudioStreamBasicDescription sasbd = src->getSampleFormat();
    manipulate_channels(chain, opts);
    // check if channel layout is available for codec
    if (opts.isAAC() || opts.isALAC())
        get_encoding_channel_layout(chain.back().get(), opts, nullptr);

    if (opts.lowpass > 0) {
        if (!SoXConvolverModule::instance().loaded())
            LOG(L"WARNING: --lowpass requires libsoxconvolver. LPF disabled\n");
        else {
            if (opts.verbose > 1 || opts.logfilename)
                LOG(L"Applying LPF: %dHz\n", opts.lowpass);
            std::shared_ptr<SoxLowpassFilter>
                f(new SoxLowpassFilter(chain.back(), opts.lowpass));
            chain.push_back(f);
        }
    }
    {
        double irate = chain.back()->getSampleFormat().mSampleRate;
        double orate = target_sample_rate(opts, chain.back().get());
        if (orate != irate) {
            if (!opts.native_resampler && SOXRModule::instance().loaded()) {
                LOG(L"%gHz -> %gHz\n", irate, orate);
                std::shared_ptr<SoxrResampler>
                    resampler(new SoxrResampler(chain.back(), orate));
                if (opts.verbose > 1 || opts.logfilename)
                    LOG(L"Using libsoxr SRC: %hs\n", resampler->engine());
                chain.push_back(resampler);
            } else {
#ifndef QAAC
                LOG(L"WARNING: --rate requires libsoxr, resampling disabled\n");
#else
                LOG(L"%gHz -> %gHz\n", irate, orate);
                AudioStreamBasicDescription sf
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
                    LOG(L"Using CoreAudio SRC: complexity %hs quality %u\n",
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
    if (opts.verbose > 1) {
        auto asbd = chain.back()->getSampleFormat();
        LOG(L"Format: %hs -> %hs\n",
            pcm_format_str(sasbd).c_str(), pcm_format_str(asbd).c_str());
    }
}

void build_filter_chain(std::shared_ptr<ISeekableSource> src,
                        std::vector<std::shared_ptr<ISource> > &chain,
                        const Options &opts)
{
    chain.push_back(src);
    build_filter_chain_sub(src, chain, opts, opts.normalize);
    if (opts.normalize && src->isSeekable()) {
        src->seekTo(0);
        Normalizer *normalizer = dynamic_cast<Normalizer*>(chain.back().get());
        double peak = normalizer->getPeak();
        chain.clear();
        chain.push_back(src);
        if (peak > FLT_MIN)
            chain.push_back(std::make_shared<Scaler>(src, 1.0/peak));
        build_filter_chain_sub(src, chain, opts, false);
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
        for (ssi = tags.begin(); ssi != tags.end(); ++ssi) {
            if (!strcasecmp(ssi->first.c_str(), "cover art")) {
                if (mp4sink && opts.copy_artwork && !opts.artworks.size()) {
                    std::vector<char> vec(ssi->second.begin(),
                                          ssi->second.end());
                    if (opts.artwork_size)
                        WICConvertArtwork(vec.data(), vec.size(),
                                          opts.artwork_size, &vec);
                    mp4sink->addArtwork(vec);
                }
            } else if (accept_tag(ssi->first))
                tagstore->setTag(ssi->first, ssi->second);
        }
        if (mp4sink) {
            IChapterParser *cp = dynamic_cast<IChapterParser*>(src);
            if (cp) {
                auto &chapters = cp->getChapters();
                if (chapters.size())
                    mp4sink->setChapters(chapters.begin(), chapters.end());
            }
        }
    }
    tagstore->setTag("encoding application",
        strutil::w2us(opts.encoder_name + L", " + encoder_config));

    for (auto uwi = opts.tagopts.begin(); uwi != opts.tagopts.end(); ++uwi) {
        const char *name = M4A::getTagNameFromFourCC(uwi->first);
        if (name)
            tagstore->setTag(name, uwi->second);
    }

    for (auto swi = opts.longtags.begin(); swi != opts.longtags.end(); ++swi)
        tagstore->setTag(swi->first, swi->second);

    if (mp4sink) {
        for (size_t i = 0; i < opts.artworks.size(); ++i)
            mp4sink->addArtwork(opts.artworks[i]);
    }
}

static
void decode_file(const std::vector<std::shared_ptr<ISource> > &chain,
                 const std::wstring &ofilename, const Options &opts)
{
    std::shared_ptr<ISink> sink;
    uint32_t chanmask = 0;
    CAFSink *cafsink = 0;
    const std::shared_ptr<ISource> src = chain.back();
    const AudioStreamBasicDescription &sf = src->getSampleFormat();
    const std::vector<uint32_t> *channels = src->getChannels();

    if (channels) {
        chanmask = chanmap::getChannelMask(*channels);
        if (opts.verbose > 1) {
            LOG(L"Output layout: %hs\n",
                chanmap::getChannelNames(*channels).c_str());
        }
    }
    if (opts.isLPCM()) {
        auto fileptr = win32::fopen(ofilename, L"wb");
        if (!opts.is_caf) {
            sink = std::make_shared<WaveSink>(fileptr, src->length(),
                                              sf, chanmask);
        } else {
            sink = std::make_shared<CAFSink>(fileptr, sf, chanmask,
                                             std::vector<uint8_t>());
            cafsink = dynamic_cast<CAFSink*>(sink.get());
            set_tags(chain[0].get(), cafsink, opts, L"");
            cafsink->beginWrite();
        }
    } else if (opts.isWaveOut()) {
        if (!chanmask)
            chanmask = chanmap::defaultChannelMask(sf.mChannelsPerFrame);
        sink = std::make_shared<WaveOutSink>(sf, chanmask);
    } else if (opts.isPeak())
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
        LOG(L"Output layout: %hs\n", chanmap::getChannelNames(vec).c_str());
    }
    return tag;
}

static
void do_encode(IEncoder *encoder, const std::wstring &ofilename,
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

    ISource *src = encoder->src();
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

static
void finalize_m4a(MP4SinkBase *sink, IEncoder *encoder,
                   const std::wstring &ofilename, const Options &opts)
{
    IEncoderStat *stat = dynamic_cast<IEncoderStat *>(encoder);
    if (opts.chapter_file) {
        try {
            double duration = stat->samplesRead() /
                encoder->getInputDescription().mSampleRate;
            auto xs = misc::convertChaptersToQT(opts.chapters, duration);
            sink->setChapters(xs.begin(), xs.end());
        } catch (const std::runtime_error &e) {
            LOG(L"WARNING: %s\n", errormsg(e).c_str());
        }
    }
    sink->writeTags();
    sink->writeBitrates(stat->overallBitrate() * 1000.0 + .5);
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
        asc = cautil::parseMagicCookieAAC(cookie);

    win32::MakeSureDirectoryPathExistsX(ofilename);
    if (opts.isMP4()) {
        std::shared_ptr<FILE> _ = win32::fopen(ofilename, L"wb");
    }
    if (opts.is_adts)
        return std::make_shared<ADTSSink>(ofilename, asc, false);
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

            AudioStreamBasicDescription iasbd =
                cautil::buildASBDForPCM(srates[i].mMinimum,
                                        channels.size(),
                                        32, kAudioFormatFlagIsFloat);
            AudioStreamBasicDescription oasbd = { 0 };
            oasbd.mFormatID = fmt;
            oasbd.mSampleRate = iasbd.mSampleRate;
            oasbd.mChannelsPerFrame = iasbd.mChannelsPerFrame;

            AudioConverterXX converter(iasbd, oasbd);
            converter.setInputChannelLayout(acl);
            converter.setOutputChannelLayout(acl);
            converter.setBitRateControlMode(
                    kAudioCodecBitRateControlMode_Constant);
            auto bits = converter.getApplicableEncodeBitRates();

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

/*
static
FARPROC WINAPI DllImportHook(unsigned notify, PDelayLoadInfo pdli)
{
    win32::throw_error(pdli->szDll, pdli->dwLastError);
    return 0;
}
*/

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
void encode_file(const std::shared_ptr<ISeekableSource> &src,
                 const std::wstring &ofilename, const Options &opts)
{
    std::vector<std::shared_ptr<ISource> > chain;
    build_filter_chain(src, chain, opts);

    if (opts.isLPCM() || opts.isWaveOut() || opts.isPeak()) {
        decode_file(chain, ofilename, opts);
        return;
    }
    uint32_t channel_layout = map_to_aac_channels(chain, opts);
    AudioStreamBasicDescription iasbd = chain.back()->getSampleFormat();
    AudioStreamBasicDescription oasbd =
        get_encoding_ASBD(chain.back().get(), opts.output_format);
    AudioConverterXX converter(iasbd, oasbd);
    AudioChannelLayout acl = { 0 };
    acl.mChannelLayoutTag = channel_layout;
    converter.setInputChannelLayout(acl);
    converter.setOutputChannelLayout(acl);
    if (opts.isAAC()) {
        int32_t quality = (opts.quality + 1) << 5;
        converter.configAACCodec(opts.method, opts.bitrate, quality);
    }
    std::wstring encoder_config = strutil::us2w(converter.getConfigAsString());
    LOG(L"%s\n", encoder_config.c_str());
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
    std::shared_ptr<ISink> sink;
    sink = open_sink(ofilename, opts, oasbd, channel_layout, cookie);
    encoder->setSink(sink);
    if (opts.isAAC()) {
        MP4Sink *mp4sink = dynamic_cast<MP4Sink*>(sink.get());
        if (mp4sink) {
            std::string params = converter.getEncodingParamsTag();
            mp4sink->setTag("Encoding Params", params);
        }
    }
    set_tags(src.get(), sink.get(), opts, encoder_config);
    CAFSink *cafsink = dynamic_cast<CAFSink*>(sink.get());
    if (cafsink)
        cafsink->beginWrite();

    do_encode(encoder.get(), ofilename, opts);
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
    std::vector<std::shared_ptr<ISource> > chain;
    build_filter_chain(src, chain, opts);

    if (opts.isLPCM() || opts.isWaveOut() || opts.isPeak()) {
        decode_file(chain, ofilename, opts);
        return;
    }
    uint32_t channel_layout = map_to_aac_channels(chain, opts);
    AudioStreamBasicDescription iasbd = chain.back()->getSampleFormat();
    AudioStreamBasicDescription oasbd =
        get_encoding_ASBD(chain.back().get(), opts.output_format);
    ALACEncoderX encoder(iasbd);
    encoder.setFastMode(opts.alac_fast);
    auto cookie = encoder.getMagicCookie();

    win32::MakeSureDirectoryPathExistsX(ofilename);
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

    do_encode(&encoder, ofilename, opts);
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
AudioStreamBasicDescription getRawFormat(const Options &opts)
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
    return asbd;
}

static
std::shared_ptr<ISeekableSource>
trim_input(std::shared_ptr<ISeekableSource> src, const Options & opts)
{
    const AudioStreamBasicDescription &asbd = src->getSampleFormat();
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

typedef std::pair<std::wstring, std::shared_ptr<ISeekableSource>> workItem;

static
void load_cue_tracks(const Options &opts, std::wstreambuf *sb, bool is_embedded,
                     const std::wstring &path, std::vector<workItem> &items)
{
    CueSheet cue;
    cue.parse(sb);
    auto tracks = cue.loadTracks(is_embedded, path, opts.cue_tracks);
    for (size_t i = 0; i < tracks.size(); ++i) {
        auto parser = dynamic_cast<ITagParser*>(tracks[i].get());
        const wchar_t *spec = opts.fname_format;
        if (!spec) spec = L"${tracknumber}${title& }${title}";
        std::wstring ofname =
            misc::generateFileName(spec, parser->getTags());
        items.push_back(std::make_pair(ofname, tracks[i]));
    }
}

static
void load_track(const wchar_t *ifilename, const Options &opts,
                std::vector<workItem> &tracks)
{
    if (strutil::wslower(PathFindExtensionW(ifilename)) == L".cue") {
        const wchar_t *base_p = PathFindFileNameW(ifilename);
        std::wstring cuedir =
            (base_p == ifilename ? L"." : std::wstring(ifilename, base_p));
        cuedir = win32::GetFullPathNameX(cuedir);
        std::wstring cuetext = misc::loadTextFile(ifilename, opts.textcp);
        std::wstringbuf istream(cuetext);
        load_cue_tracks(opts, &istream, false, cuedir, tracks);
        return;
    }

    std::wstring ofilename(ifilename);
    auto src = InputFactory::instance().open(ifilename);
    auto parser = dynamic_cast<ITagParser*>(src.get());
    if (parser) {
        auto meta = parser->getTags();
        auto cue = meta.find("CUESHEET");
        if (cue != meta.end()) {
            try {
                std::wstringbuf wsb(strutil::us2w(cue->second));
                load_cue_tracks(opts, &wsb, true, ifilename, tracks);
                return;
            } catch (...) {}
        }
        if (opts.filename_from_tag && opts.fname_format) {
            auto fn = misc::generateFileName(opts.fname_format, meta);
            if (fn.size()) ofilename = fn + L".stub";
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
            LOG(L"WARNING: %s\n", errormsg(e).c_str());
        }
    }
    for (auto it = opts->ftagopts.begin(); it != opts->ftagopts.end(); ++it) {
        try {
            opts->tagopts[it->first] =
                strutil::w2us(misc::loadTextFile(it->second));
        } catch (const std::exception &e) {
            LOG(L"WARNING: %s\n", errormsg(e).c_str());
        }
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
std::wstring get_output_filename(const std::wstring &ifilename,
                                 const Options &opts)
{
    if (opts.ofilename) return opts.ofilename;

    const wchar_t *ext = opts.extension();
    const wchar_t *outdir = opts.outdir ? opts.outdir : L".";
    if (!std::wcscmp(ifilename.c_str(), L"-"))
        return std::wstring(L"stdin") + ext;

    std::wstring obasename = win32::PathReplaceExtension(ifilename, ext);
    std::wstring ofilename = strutil::format(L"%s/%s", outdir,
                                             obasename.c_str());

    /* test if ifilename and ofilename refer to the same file */
    std::shared_ptr<FILE> ifp, ofp;
    try {
        ifp = win32::fopen(ifilename, L"rb");
        ofp = win32::fopen(ofilename, L"rb");
    } catch (...) {
        return ofilename;
    }
    if (!win32::is_same_file(_fileno(ifp.get()), _fileno(ofp.get())))
        return ofilename;

    std::wstring tl = strutil::format(L"_%s", ext);
    return win32::PathReplaceExtension(ofilename, tl.c_str());
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
    auto cmdline = std::wstring(GetCommandLineW()) + L"\n";
    OutputDebugStringW(cmdline.c_str());

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
    Log &logger = Log::instance();

    try {
        ConsoleTitleSaver consoleTitle;

        if (opts.verbose)
            logger.enable_stderr();
        if (opts.logfilename)
            logger.enable_file(opts.logfilename);

        if (opts.nice)
            SetPriorityClass(GetCurrentProcess(), IDLE_PRIORITY_CLASS);

        std::string encoder_name;
        encoder_name = strutil::format(PROGNAME " %s", get_qaac_version());
#ifdef QAAC
//      decltype(__pfnDliNotifyHook2) __pfnDliFailureHook2 = DllImportHook;
        set_dll_directories(opts.verbose);
        HMODULE hDll = LoadLibraryW(L"CoreAudioToolbox.dll");
        if (!hDll)
            win32::throw_error("CoreAudioToolbox.dll", GetLastError());
        else {
            WORD langid_us = MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US);
            std::string ver = win32::get_dll_version_for_locale(hDll, langid_us);
            encoder_name = strutil::format("%s, CoreAudioToolbox %s",
                                           encoder_name.c_str(), ver.c_str());
            setup_aach_codec(hDll);
            FreeLibrary(hDll);
        }
#endif
        opts.encoder_name = strutil::us2w(encoder_name);
        if (!opts.print_available_formats)
            LOG(L"%s\n", opts.encoder_name.c_str());

        if (opts.check_only) {
            if (SoXConvolverModule::instance().loaded())
                LOG(L"libsoxconvolver %hs\n",
                    SoXConvolverModule::instance().version());
            if (SOXRModule::instance().loaded())
                LOG(L"%hs\n", SOXRModule::instance().version());
            if (LibSndfileModule::instance().loaded())
                LOG(L"%hs\n", LibSndfileModule::instance().version_string());
            if (FLACModule::instance().loaded())
                LOG(L"libFLAC %hs\n", FLACModule::instance().VERSION_STRING);
            if (WavpackModule::instance().loaded())
                LOG(L"wavpackdll %hs\n",
                    WavpackModule::instance().GetLibraryVersionString());
            if (TakModule::instance().loaded()) {
                TtakInt32 var, comp;
                TakModule::instance().GetLibraryVersion(&var, &comp);
                LOG(L"tak_deco_lib %u.%u.%u %hs\n",
                        var >> 16, (var >> 8) & 0xff, var & 0xff,
                        TakModule::instance().compatible() ? "compatible"
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
            env += opts.tmpdir;
            _wputenv(env.c_str());
        }

        if (opts.ofilename) {
            std::wstring fullpath = win32::GetFullPathNameX(opts.ofilename);
            const wchar_t *ws = fullpath.c_str();
            if (!std::wcscmp(opts.ofilename, L"-"))
                _setmode(1, _O_BINARY);
        }

        if (opts.sort_args) {
            std::sort(&argv[0], &argv[argc],
                      [](const wchar_t *a, const wchar_t *b) {
                          return std::wcscmp(a, b) < 0;
                      });
        }
        SetConsoleCtrlHandler(console_interrupt_handler, TRUE);

        if (opts.is_raw) {
            InputFactory::instance().setRawFormat(getRawFormat(opts));
        }
        InputFactory::instance().setIgnoreLength(opts.ignore_length);

        struct CleanupScope {
            ~CleanupScope() {
                InputFactory::instance().close();
                WaveOutDevice::instance().close();
            }
        } __cleanup__;

        std::vector<workItem> workItems;
        for (int i = 0; i < argc; ++i)
            load_track(argv[i], opts, workItems);

        if (!opts.concat) {
            for (size_t i = 0; i < workItems.size() && !g_interrupted; ++i) {
                std::wstring ofilename =
                    get_output_filename(workItems[i].first, opts);
                LOG(L"\n%s\n",
                    ofilename == L"-" ? L"<stdout>"
                                      : PathFindFileNameW(ofilename.c_str()));
                auto src = trim_input(workItems[i].second, opts);
                src->seekTo(0);
                encode_file(src, ofilename, opts);
            }
        } else {
            std::wstring ofilename = get_output_filename(argv[0], opts);
            LOG(L"\n%s\n",
                ofilename == L"-" ? L"<stdout>"
                                  : PathFindFileNameW(ofilename.c_str()));

            auto cs = std::make_shared<CompositeSource>();
            for (size_t i = 0; i < workItems.size(); ++i)
                cs->addSourceWithChapter(workItems[i].second, L"");

            auto src = trim_input(cs, opts);
            src->seekTo(0);
            encode_file(src, ofilename, opts);
        }
    } catch (const std::exception &e) {
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
