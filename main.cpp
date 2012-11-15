#include <iostream>
#include <ctime>
#include <cstdarg>
#include <clocale>
#include <algorithm>
#include <functional>
#include "strutil.h"
#include "win32util.h"
#include <shellapi.h>
#include "itunetags.h"
#include "options.h"
#include "inputfactory.h"
#include "sink.h"
#include "alacsink.h"
#include "wavsink.h"
#include "cuesheet.h"
#include "composite.h"
#include "nullsource.h"
#include "soxdsp.h"
#include "normalize.h"
#include "mixer.h"
#include "Quantizer.h"
#include "scaler.h"
#include "pipedreader.h"
#include "TrimmedSource.h"
#include "chanmap.h"
#include "logging.h"
#include "textfile.h"
#include "expand.h"
#ifdef REFALAC
#include "alacenc.h"
#else
#include <delayimp.h>
#include "AudioCodecX.h"
#include "CoreAudioEncoder.h"
#include "CoreAudioResampler.h"
#endif
#include <ShlObj.h>
#include <crtdbg.h>

#ifdef REFALAC
#define PROGNAME "refalac"
#else
#define PROGNAME "qaac"
#endif

inline
std::wstring errormsg(const std::exception &ex)
{
    return strutil::us2w(ex.what());
}

std::wstring get_module_directory()
{
    std::wstring selfpath = win32::GetModuleFileNameX(0);
    const wchar_t *fpos = PathFindFileNameW(selfpath.c_str());
    return selfpath.substr(0, fpos - selfpath.c_str());
}

static
void load_lyrics_file(Options *opts)
{
    try {
	std::map<uint32_t, std::wstring>::iterator it
	    = opts->tagopts.find(Tag::kLyrics);
	if (it != opts->tagopts.end())
	    it->second = load_text_file(it->second.c_str(), opts->textcp);
    } catch (const std::exception &e) {
	LOG(L"WARNING: %s\n", errormsg(e).c_str());
    }
}

static
std::wstring get_output_filename(const wchar_t *ifilename, const Options &opts)
{
    if (opts.ofilename) {
	if (!std::wcscmp(opts.ofilename, L"-"))
	    return opts.ofilename;
	else
	    return win32::GetFullPathNameX(opts.ofilename);
    }
    const wchar_t *ext = opts.extension();
    const wchar_t *outdir = opts.outdir ? opts.outdir : L".";
    if (!std::wcscmp(ifilename, L"-"))
	return std::wstring(L"stdin.") + ext;
    else {
	std::wstring obasename =
	    win32::PathReplaceExtension(PathFindFileNameW(ifilename), ext);
	std::wstring ofilename = 
	    win32::GetFullPathNameX(strutil::format(L"%s/%s", outdir,
						    obasename.c_str()));
	if (win32::GetFullPathNameX(ifilename) == ofilename) {
	    std::string codec_name(util::fourcc(opts.output_format));
	    while (codec_name.size() && codec_name.back() == ' ')
		codec_name.pop_back();
	    std::wstring tl =
		strutil::format(L"%hs.%s", codec_name.c_str(), ext);
	    ofilename = win32::PathReplaceExtension(ofilename, tl.c_str());
	}
	return ofilename;
    }
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
    uint32_t m_last_tick;
    std::wstring m_message;
    bool m_verbose;
    bool m_console_visible;
public:
    PeriodicDisplay(uint32_t interval, bool verbose=true)
	: m_interval(interval),
	  m_verbose(verbose),
	  m_last_tick(GetTickCount())
    {
	m_console_visible = IsWindowVisible(GetConsoleWindow());
    }
    void put(const std::wstring &message) {
	m_message = message;
	uint32_t tick = GetTickCount();
	if (tick - m_last_tick > m_interval) {
	    flush();
	    m_last_tick = tick;
	}
    }
    void flush() {
	if (m_verbose) std::fputws(m_message.c_str(), stderr);
	if (m_console_visible) {
	    std::vector<wchar_t> s(m_message.size() + 1);
	    std::wcscpy(&s[0], m_message.c_str());
	    strutil::squeeze(&s[0], L"\r");
	    SetConsoleTitleW(strutil::format(L"%hs %s", PROGNAME, &s[0]).c_str());
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
public:
    Progress(bool verbosity, uint64_t total, uint32_t rate)
	: m_disp(100, verbosity), m_verbose(verbosity),
	  m_total(total), m_rate(rate)
    {
	m_console_visible = IsWindowVisible(GetConsoleWindow());
	if (total != ~0ULL)
	    m_tstamp = formatSeconds(static_cast<double>(total) / rate);
    }
    void update(uint64_t current)
    {
	if (!m_verbose && !m_console_visible) return;
	double fcurrent = current;
	double percent = 100.0 * fcurrent / m_total;
	double seconds = fcurrent / m_rate;
	double ellapsed = m_timer.ellapsed();
	double eta = ellapsed * (m_total / fcurrent - 1);
	double speed = ellapsed ? seconds/ellapsed : 0.0;
	if (m_total == ~0ULL)
	    m_disp.put(strutil::format(L"\r%s (%.1fx)   ",
		formatSeconds(seconds).c_str(), speed));
	else
	    m_disp.put(strutil::format(L"\r[%.1f%%] %s/%s (%.1fx), ETA %s  ",
		percent, formatSeconds(seconds).c_str(), m_tstamp.c_str(),
		speed, formatSeconds(eta).c_str()));
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
	while (encoder->encodeChunk(1)) {
	    progress.update(src->getPosition());
	    if (statfp)
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

static
void write_tags(MP4FileX *mp4file, const Options &opts, ISource *src,
	IEncoder *encoder, const std::wstring &encoder_config)
{
    TagEditor editor;

    /*
     * At this point, encoder's input format might not be the same with
     * original source's format (src points to the actual source).
     */
    const AudioStreamBasicDescription
	&iformat = encoder->getInputDescription(),
	&oformat = encoder->getOutputDescription();

    ITagParser *parser = dynamic_cast<ITagParser*>(src);
    IEncoderStat *stat = dynamic_cast<IEncoderStat*>(encoder);

    if (parser) {
	editor.setTag(parser->getTags());
	const std::vector<chapters::entry_t> *chapters = parser->getChapters();
	if (chapters) {
	    editor.setChapters(*chapters);
	}
    }
    editor.setTag(opts.tagopts);
    editor.setLongTag(opts.longtags);
    editor.setTag(Tag::kTool, opts.encoder_name + L", " + encoder_config);
#ifndef REFALAC
    if (opts.isAAC() && stat->samplesWritten()) {
	CoreAudioEncoder *caencoder =
	    dynamic_cast<CoreAudioEncoder*>(encoder);
	editor.setGaplessInfo(caencoder->getGaplessInfo());
    }
#endif
    editor.setArtworkSize(opts.artwork_size);
    for (size_t i = 0; i < opts.artworks.size(); ++i)
	editor.addArtwork(opts.artworks[i].c_str());

    if (opts.chapter_file) {
	std::vector<chapters::abs_entry_t> abs_entries;
	std::vector<chapters::entry_t> chapters;
	try {
	    chapters::load_from_file(opts.chapter_file, &abs_entries,
				     opts.textcp);
	    double duration = stat->samplesRead() / iformat.mSampleRate;
	    chapters::abs_to_duration(abs_entries, &chapters, duration);
	    editor.setChapters(chapters);
	} catch (const std::runtime_error &e) {
	    LOG(L"WARNING: %s\n", errormsg(e).c_str());
	}
    }
    editor.save(*mp4file);
    try {
	editor.saveArtworks(*mp4file);
    } catch (const std::exception &e) {
	LOG(L"WARNING: %s\n", errormsg(e).c_str());
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
	    disp.put(strutil::format(L"\r%llu/%llu chunks written (optimizing)",
			    i, total).c_str());
	}
	disp.flush();
	if (verbose) std::putwc(L'\n', stderr);
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
    while ((rc = normalizer->process(4096)) > 0) {
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
    search_paths.push_back(get_module_directory());
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
	      const Options &opts, uint32_t *wav_chanmask,
	      uint32_t *aac_layout, bool threading)
{
    uint32_t nchannels = chain.back()->getSampleFormat().mChannelsPerFrame;
    const std::vector<uint32_t> *channels = chain.back()->getChannels();
    if (channels) {
	if (opts.verbose > 1) {
	    LOG(L"Input layout: %hs\n",
		chanmap::getChannelNames(*channels).c_str());
	}
	// reorder to Microsoft (USB) order
	std::vector<uint32_t> work;
	chanmap::convertFromAppleLayout(*channels, &work);
	std::vector<uint32_t> mapping;
	chanmap::getMappingToUSBOrder(work, &mapping);
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
	if (!input::factory()->libsoxrate.loaded())
	    LOG(L"WARNING: mixer requires libsoxrate. Mixing disabled\n");
	else {
	    std::vector<std::vector<complex_t> > matrix;
	    matrix_from_preset(opts, &matrix);
	    if (opts.verbose > 1 || opts.logfilename) {
		LOG(L"Matrix mixer: %uch -> %uch\n",
		    static_cast<uint32_t>(matrix[0].size()),
		    static_cast<uint32_t>(matrix.size()));
	    }
	    std::shared_ptr<ISource>
		mixer(new MatrixMixer(chain.back(), input::factory()->libsoxrate,
				      matrix, threading,
				      !opts.no_matrix_normalize));
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
    *wav_chanmask = chanmask;
    if (chanmask) {
	if (opts.isLPCM() && opts.verbose > 1) {
	    std::vector<uint32_t> vec;
	    chanmap::getChannels(chanmask, &vec);
	    LOG(L"Output layout: %hs\n",
		chanmap::getChannelNames(vec).c_str());
	}
    }
    if (!opts.isLPCM()) {
	// construct mapped channel layout to AAC/ALAC order
	*aac_layout = chanmap::AACLayoutFromBitmap(chanmask);
	std::vector<uint32_t> aacmap;
	chanmap::getMappingToAAC(chanmask, &aacmap);
	if (aacmap.size()) {
	    std::shared_ptr<ISource>
		mapper(new ChannelMapper(chain.back(), aacmap));
	    chain.push_back(mapper);
	}
	if (opts.verbose > 1) {
	    std::vector<uint32_t> vec;
	    AudioChannelLayout acl = { 0 };
	    acl.mChannelLayoutTag = *aac_layout;
	    chanmap::getChannels(&acl, &vec);
	    LOG(L"Output layout: %hs\n", chanmap::getChannelNames(vec).c_str());
	}
    }
}

static
std::shared_ptr<ISeekableSource>
delayed_source(const std::shared_ptr<ISeekableSource> &src,
	       const Options & opts)
{
    const AudioStreamBasicDescription &asbd = src->getSampleFormat();
    double rate = asbd.mSampleRate;
    if (opts.delay > 0) {
	int nsamples = lrint(0.001 * opts.delay * rate);
	if (opts.verbose > 1 || opts.logfilename)
	    LOG(L"Delay of %dms: pad %d samples\n", opts.delay, nsamples);
	std::shared_ptr<CompositeSource> cp(new CompositeSource());
	std::shared_ptr<ISeekableSource> ns(new NullSource(asbd));
	cp->addSource(std::make_shared<TrimmedSource>(ns, 0, nsamples));
	cp->addSource(src);
	return cp;
    } else if (opts.delay < 0) {
	int nsamples = lrint(-0.001 * opts.delay * rate);
	if (opts.verbose > 1 || opts.logfilename)
	    LOG(L"Delay of %dms: truncate %d samples\n", opts.delay, nsamples);
	return std::make_shared<TrimmedSource>(src, nsamples, ~0ULL);
    }
    return src;
}

#ifndef REFALAC
static
void config_aac_codec(AudioConverterX &converter, const Options &opts);

inline uint32_t bound_quality(uint32_t n)
{
    return std::min(n, static_cast<uint32_t>(kAudioConverterQuality_Max));
}

#endif

void build_filter_chain_sub(std::shared_ptr<ISeekableSource> src,
			    std::vector<std::shared_ptr<ISource> > &chain,
			    const Options &opts, uint32_t *wChanmask,
			    uint32_t *aacLayout,
			    AudioStreamBasicDescription *inputDesc,
			    AudioStreamBasicDescription *outputDesc,
			    bool normalize_pass=false)
{
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    bool threading = opts.threading && si.dwNumberOfProcessors > 1;

    AudioStreamBasicDescription sasbd = src->getSampleFormat();

#ifndef REFALAC
    std::shared_ptr<AudioCodecX> codec;
    if (!opts.isLPCM())
	codec.reset(new AudioCodecX(opts.output_format));
#endif
    mapped_source(chain, opts, wChanmask, aacLayout, threading);

    if (!opts.isLPCM()) {
#ifndef REFALAC
	if (!codec->isAvailableOutputChannelLayout(*aacLayout))
	    throw std::runtime_error("Channel layout not supported");
#else
	switch (*aacLayout) {
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
	if (!input::factory()->libsoxrate.loaded())
	    LOG(L"WARNING: --lowpass requires libsoxrate. LPF disabled\n");
        else {
	    if (opts.verbose > 1 || opts.logfilename)
		LOG(L"Applying LPF: %dHz\n", opts.lowpass);
	    std::shared_ptr<ISoxDSPEngine>
		engine(new SoxLowpassFilter(input::factory()->libsoxrate,
					    chain.back()->getSampleFormat(),
					    opts.lowpass,
					    threading));
	    std::shared_ptr<ISource>
		processor(new SoxDSPProcessor(engine, chain.back()));
	    chain.push_back(processor);
	}
    }
    AudioStreamBasicDescription iasbd = chain.back()->getSampleFormat();
    AudioStreamBasicDescription oasbd = { 0 };
    oasbd.mFormatID = opts.output_format;
    oasbd.mChannelsPerFrame = iasbd.mChannelsPerFrame;

    double rate = opts.rate > 0 ? opts.rate
			        : opts.rate == -1 ? iasbd.mSampleRate
						  : 0;
    if (!opts.isAAC()) {
	oasbd.mSampleRate = rate > 0 ? rate : iasbd.mSampleRate;
    }
#ifndef REFALAC
    else {
	if (rate > 0)
	    oasbd.mSampleRate =
		codec->getClosestAvailableOutputSampleRate(rate);
	else {
	    AudioChannelLayout acl = { 0 };
	    acl.mChannelLayoutTag = *aacLayout;
	    AudioConverterX converter(iasbd, oasbd);
	    converter.setInputChannelLayout(acl);
	    converter.setOutputChannelLayout(acl);
	    config_aac_codec(converter, opts);
	    converter.getOutputStreamDescription(&oasbd);
	}
    }
#endif
    if (oasbd.mSampleRate != iasbd.mSampleRate) {
	LOG(L"%gHz -> %gHz\n", iasbd.mSampleRate, oasbd.mSampleRate);
	if (!opts.native_resampler && input::factory()->libsoxrate.loaded()) {
	    if (opts.verbose > 1 || opts.logfilename)
		LOG(L"Using libsoxrate SRC\n");
	    std::shared_ptr<ISoxDSPEngine>
		engine(new SoxResampler(input::factory()->libsoxrate,
					chain.back()->getSampleFormat(),
					oasbd.mSampleRate,
					threading));
	    std::shared_ptr<ISource>
		processor(new SoxDSPProcessor(engine, chain.back()));
	    chain.push_back(processor);
	} else {
#ifdef REFALAC
	    oasbd.mSampleRate = iasbd.mSampleRate;
	    LOG(L"WARNING: --rate requires libsoxrate\n");
#else
	    if (opts.native_resampler_quality >= 0 ||
		opts.native_resampler_complexity >= 0 ||
		opts.isLPCM())
	    {
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
    if (normalize_pass) {
	do_normalize(chain, opts, src->isSeekable());
	if (src->isSeekable())
	    return;
    }

    if (opts.gain) {
	double scale = dB_to_scale(opts.gain);
	if (opts.verbose > 1 || opts.logfilename)
	    LOG(L"Gain adjustment: %gdB, scale factor %g\n",
		opts.gain, scale);
	std::shared_ptr<ISource> scaler(new Scaler(chain.back(), scale));
	chain.push_back(scaler);
    }
    if (opts.bits_per_sample) {
	if (opts.isAAC())
	    LOG(L"WARNING: --bits-per-sample has no effect for AAC\n");
	else if (chain.back()->getSampleFormat().mBitsPerChannel
		 != opts.bits_per_sample) {
	    bool is_float = (opts.bits_per_sample == 32 && opts.isLPCM());
	    std::shared_ptr<ISource>
		isrc(new Quantizer(chain.back(), opts.bits_per_sample,
				   opts.no_dither, is_float));
	    chain.push_back(isrc);
	    if (opts.verbose > 1 || opts.logfilename)
		LOG(L"Convert to %d bit\n", opts.bits_per_sample);
	}
    } else if (opts.isALAC()) {
	bool isfloat = (chain.back()->getSampleFormat().mFormatFlags
			& kAudioFormatFlagIsFloat);
	/* 
	 * When converted to float by DSP, automatically quantize to integer.
	 */
	if ((sasbd.mFormatFlags & kAudioFormatFlagIsSignedInteger) && isfloat) {
	    unsigned bits = ((sasbd.mBitsPerChannel + 3) & ~3);
	    if (bits > 24) bits = 32;
	    chain.push_back(std::make_shared<Quantizer>(chain.back(), bits,
						        opts.no_dither));
	} else if (isfloat) {
	    // Don't automatically quantize float input
	    throw std::runtime_error("ALAC: input format is not supported");
	}
    } else if (opts.isLPCM()) {
	/* output f64 sample only when input was already f64 */
	if (sasbd.mBitsPerChannel <= 32 &&
	    chain.back()->getSampleFormat().mBitsPerChannel > 32)
	{
	    chain.push_back(std::make_shared<Quantizer>(chain.back(), 32,
							opts.no_dither, true));
	}
    }
    if (threading && !opts.isLPCM()) {
	PipedReader *reader = new PipedReader(chain.back());
	reader->start();
	chain.push_back(std::shared_ptr<ISource>(reader));
	if (opts.verbose > 1 || opts.logfilename)
	    LOG(L"Enable threading\n");
    }
    iasbd = chain.back()->getSampleFormat();
    if (opts.isALAC()) {
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
	    throw std::runtime_error("ALAC: Not supported bit depth");
	}
    }
    if (inputDesc) *inputDesc = iasbd;
    if (outputDesc) *outputDesc = oasbd;
}

void build_filter_chain(std::shared_ptr<ISeekableSource> src,
		        std::vector<std::shared_ptr<ISource> > &chain,
		        const Options &opts, uint32_t *wChanmask,
		        uint32_t *aacLayout,
		        AudioStreamBasicDescription *inputDesc,
		        AudioStreamBasicDescription *outputDesc)
{
    chain.push_back(src);
    build_filter_chain_sub(src, chain, opts, wChanmask, aacLayout,
			   inputDesc, outputDesc, opts.normalize);
    if (opts.normalize && src->isSeekable()) {
	src->seekTo(0);
	Normalizer *normalizer = dynamic_cast<Normalizer*>(chain.back().get());
	double peak = normalizer->getPeak();
	chain.clear();
	chain.push_back(src);
	if (peak > FLT_MIN)
	    chain.push_back(std::make_shared<Scaler>(src, 1.0/peak));
	build_filter_chain_sub(src, chain, opts, wChanmask, aacLayout,
			       inputDesc, outputDesc, false);
    }
}

static
void decode_file(const std::vector<std::shared_ptr<ISource> > &chain,
		 const std::wstring &ofilename, const Options &opts,
		 uint32_t chanmask)
{
    std::shared_ptr<FILE> fileptr = win32::fopen(ofilename, L"wb");

    const std::shared_ptr<ISource> src = chain.back();
    const AudioStreamBasicDescription &sf = src->getSampleFormat();
    WaveSink sink(fileptr.get(), src->length(), sf, chanmask);

    Progress progress(opts.verbose, src->length(),
		      src->getSampleFormat().mSampleRate);

    uint32_t bpf = sf.mBytesPerFrame;
    std::vector<uint8_t> buffer(4096 * bpf);
    try {
	size_t nread;
	while ((nread = src->readSamples(&buffer[0], 4096)) > 0) {
	    progress.update(src->getPosition());
	    sink.writeSamples(&buffer[0], nread * bpf, nread);
	}
	progress.finish(src->getPosition());
    } catch (const std::exception &e) {
	LOG(L"\nERROR: %s\n", errormsg(e).c_str());
    }
    sink.finishWrite();
}

#ifndef REFALAC
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
std::shared_ptr<ISink> open_sink(const std::wstring &ofilename,
	const Options &opts, const std::vector<uint8_t> &cookie)
{
    ISink *sink;
    if (opts.is_adts)
	sink = new ADTSSink(ofilename, cookie);
    else if (opts.isALAC())
	sink = new ALACSink(ofilename, cookie, !opts.no_optimize);
    else if (opts.isAAC())
	sink = new MP4Sink(ofilename, cookie, opts.output_format,
			   !opts.no_optimize);
    return std::shared_ptr<ISink>(sink);
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
		std::wprintf(L"%c%g", delim, bits[k].mMinimum / 1000.0);
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
    std::wstring dir = get_module_directory() + L"QTfiles";
    searchPaths = strutil::format(L"%s;%s", dir.c_str(), searchPaths.c_str());
    SetEnvironmentVariableW(L"PATH", searchPaths.c_str());
}

static
void config_aac_codec(AudioConverterX &converter, const Options &opts)
{
    converter.setBitRateControlMode(opts.method);
    if (opts.method != Options::kTVBR) {
	double bitrate = opts.bitrate ? opts.bitrate * 1000 : 0x7fffffff;
	bitrate = converter.getClosestAvailableBitRate(bitrate);
	converter.setEncodeBitRate(bitrate);
    } else {
	converter.setSoundQualityForVBR(bound_quality(opts.bitrate));
    }
    converter.setCodecQuality(bound_quality((opts.quality + 1) << 5));
}

static
void encode_file(const std::shared_ptr<ISeekableSource> &src,
	const std::wstring &ofilename, const Options &opts)
{
    uint32_t wavChanmask;
    uint32_t aacLayout;
    AudioStreamBasicDescription iasbd, oasbd;

    std::vector<std::shared_ptr<ISource> > chain;
    build_filter_chain(src, chain, opts, &wavChanmask, &aacLayout,
		       &iasbd, &oasbd);
    if (opts.isLPCM()) {
	decode_file(chain, ofilename, opts, wavChanmask);
	return;
    }
    AudioConverterX converter(iasbd, oasbd);
    AudioChannelLayout acl = { 0 };
    acl.mChannelLayoutTag = aacLayout;
    converter.setInputChannelLayout(acl);
    converter.setOutputChannelLayout(acl);
    if (opts.isAAC()) config_aac_codec(converter, opts);

    std::wstring encoder_config = get_encoder_config(converter);
    LOG(L"%s\n", encoder_config.c_str());
    std::vector<uint8_t> cookie;
    converter.getCompressionMagicCookie(&cookie);
    CoreAudioEncoder encoder(converter);
    encoder.setSource(chain.back());
    std::shared_ptr<ISink> sink = open_sink(ofilename, opts, cookie);
    encoder.setSink(sink);
    do_encode(&encoder, ofilename, chain, opts);
    LOG(L"Overall bitrate: %gkbps\n", encoder.overallBitrate());
    MP4SinkBase *asink = dynamic_cast<MP4SinkBase*>(sink.get());
    if (asink) {
	write_tags(asink->getFile(), opts, src.get(), &encoder,
		   encoder_config);
	if (!opts.no_optimize)
	    do_optimize(asink->getFile(), ofilename, opts.verbose > 1);
	asink->close();
    }
}
#else // REFALAC

static
void encode_file(const std::shared_ptr<ISeekableSource> &src,
	const std::wstring &ofilename, const Options &opts)
{
    uint32_t wavChanmask;
    uint32_t aacLayout;
    AudioStreamBasicDescription iasbd;
    std::vector<std::shared_ptr<ISource> > chain;
    build_filter_chain(src, chain, opts, &wavChanmask, &aacLayout, &iasbd, 0);

    if (opts.isLPCM()) {
	decode_file(chain, ofilename, opts, wavChanmask);
	return;
    }
    ALACEncoderX encoder(iasbd);
    encoder.setFastMode(opts.alac_fast);
    std::vector<uint8_t> cookie;
    encoder.getMagicCookie(&cookie);

    std::shared_ptr<ISink> sink(new ALACSink(ofilename, cookie,
					     !opts.no_optimize));
    encoder.setSource(chain.back());
    encoder.setSink(sink);
    do_encode(&encoder, ofilename, chain, opts);
    ALACSink *asink = dynamic_cast<ALACSink*>(sink.get());
    LOG(L"Overall bitrate: %gkbps\n", encoder.overallBitrate());
    write_tags(asink->getFile(), opts, src.get(), &encoder,
	       L"Apple Lossless Encoder");
    if (!opts.no_optimize)
	do_optimize(asink->getFile(), ofilename, opts.verbose > 1);
}
#endif

const char *get_qaac_version();

static
void setup_input_factory(const Options &opts)
{
    input::InputFactory *factory = input::factory();

    factory->libsndfile = LibSndfileModule(L"libsndfile-1.dll");
    factory->libflac = FLACModule(L"libFLAC.dll");
    if (!factory->libflac.loaded())
	factory->libflac = FLACModule(L"libFLAC-8.dll");
    factory->libwavpack = WavpackModule(L"wavpackdll.dll");
    if (!factory->libwavpack.loaded())
	factory->libwavpack = WavpackModule(L"libwavpack-1.dll");
    factory->libtak = TakModule(L"tak_deco_lib.dll");
#ifdef _WIN64
    factory->libsoxrate = SoxModule(L"libsoxrate64.dll");
#else
    factory->libsoxrate = SoxModule(L"libsoxrate.dll");
#endif

    if (opts.is_raw) {
	AudioStreamBasicDescription asbd;
	getRawFormat(opts, &asbd);
	factory->setRawFormat(asbd);
    }
    factory->setIgnoreLength(opts.ignore_length);
}

static
void load_cue_sheet(const wchar_t *ifilename, const Options &opts,
		      std::vector<chapters::Track> &tracks)
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
	        std::vector<chapters::Track> &tracks)
{
    const wchar_t *name = L"stdin";
    if (std::wcscmp(ifilename, L"-"))
	name = PathFindFileNameW(ifilename);
    std::wstring title(name, PathFindExtensionW(name));

    std::shared_ptr<ISeekableSource>
	src(input::factory()->open(ifilename));

    ITagParser *parser = dynamic_cast<ITagParser*>(src.get());
    if (parser) {
	const std::map<uint32_t, std::wstring> &meta =
	    parser->getTags();
	std::map<uint32_t, std::wstring>::const_iterator it
	    = meta.find(Tag::kTitle);
	if (it != meta.end())
	    title = it->second;
    }
    chapters::Track new_track;
    new_track.name = title;
    new_track.source = src;
    new_track.ofilename = name;
    tracks.push_back(new_track);
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

    SetDllDirectoryW(L"");
    std::setlocale(LC_CTYPE, "");
    std::setbuf(stderr, 0);
    _setmode(0, _O_BINARY);
    _setmode(2, _O_U8TEXT);

#if 0
    FILE *fp = std::fopen("CON", "r");
    std::getc(fp);
#endif
    int result = 0;
    if (!opts.parse(argc, argv))
	return 1;
    opts.is_console_visible = IsWindowVisible(GetConsoleWindow());

    COMInitializer __com__;
    try {
	ConsoleTitleSaver consoleTitle;

	if (opts.verbose && !opts.print_available_formats)
	    Log::instance()->enable_stderr();
	if (opts.logfilename && !opts.print_available_formats)
	    Log::instance()->enable_file(opts.logfilename);

	if (opts.nice)
	    SetPriorityClass(GetCurrentProcess(), IDLE_PRIORITY_CLASS);

	std::string encoder_name;
	encoder_name = strutil::format(PROGNAME " %s", get_qaac_version());
#ifndef REFALAC
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

	setup_input_factory(opts);
	input::InputFactory *factory = input::factory();

	if (opts.check_only) {
	    if (factory->libsoxrate.loaded())
		LOG(L"libsoxrate %hs\n",
		    factory->libsoxrate.version_string());
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
#ifndef REFALAC
	if (opts.print_available_formats) {
	    show_available_aac_settings();
	    return 0;
	}
#endif
	mp4v2::impl::log.setVerbosity(MP4_LOG_NONE);
	//mp4v2::impl::log.setVerbosity(MP4_LOG_VERBOSE4);

	load_lyrics_file(&opts);
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
		_dup2(win32::create_named_pipe(ws), 1);
		_setmode(1, _O_BINARY);
	    }
	}

	std::vector<chapters::Track> tracks;
	const wchar_t *ifilename = 0;

	for (int i = 0; i < argc; ++i) {
	    ifilename = argv[i];
	    if (strutil::wslower(PathFindExtensionW(ifilename)) == L".cue")
		load_cue_sheet(ifilename, opts, tracks);
	    else
		load_track(ifilename, opts, tracks);
	}
	if (!opts.concat) {
	    for (size_t i = 0; i < tracks.size(); ++i) {
		chapters::Track &track = tracks[i];
		std::wstring ofilename =
		    get_output_filename(track.ofilename.c_str(), opts);
		LOG(L"\n%s\n", PathFindFileNameW(ofilename.c_str()));
		std::shared_ptr<ISeekableSource> src =
		    delayed_source(track.source, opts);
		src->seekTo(0);
		encode_file(src, ofilename, opts);
	    }
	} else {
	    std::wstring ofilename = get_output_filename(ifilename, opts);
	    LOG(L"\n%s\n", PathFindFileNameW(ofilename.c_str()));
	    std::shared_ptr<CompositeSource> cs
		= std::make_shared<CompositeSource>();
	    for (size_t i = 0; i < tracks.size(); ++i) {
		chapters::Track &track = tracks[i];
		cs->addSourceWithChapter(track.source, track.name);
	    }
	    std::shared_ptr<ISeekableSource> src(delayed_source(cs, opts));
	    src->seekTo(0);
	    encode_file(src, ofilename, opts);
	}
    } catch (const std::exception &e) {
	if (opts.print_available_formats)
	    Log::instance()->enable_stderr();
	LOG(L"ERROR: %s\n", errormsg(e).c_str());
	result = 2;
    }
    delete Log::instance();
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
