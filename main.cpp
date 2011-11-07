#include <iostream>
#include <ctime>
#include <cstdarg>
#include <clocale>
#include <algorithm>
#include <functional>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef NO_QT
#include <QTML.h>
#include <QTLoadLibraryUtils.h>
#endif
#include "strcnv.h"
#include "win32util.h"
#include <shellapi.h>
#ifndef NO_QT
#include "SCAudioEncoder.h"
#include "qtmoviesource.h"
#include "reg.h"
#include "aacconfig.h"
#else
#include "alacenc.h"
#include "alacsrc.h"
#include "wavsink.h"
#endif
#include "itunetags.h"
#include "sink.h"
#include "rawsource.h"
#include "flacsrc.h"
#include "alacsink.h"
#include "options.h"
#include "cuesheet.h"
#include "composite.h"
#include "nullsource.h"
#include "wavsource.h"
#include "expand.h"
#include "resampler.h"
#include "logging.h"
#include "textfile.h"
#include <crtdbg.h>

#ifdef NO_QT
#define PROGNAME "refalac"
#else
#define PROGNAME "qaac"
#endif

std::wstring get_module_directory()
{
    std::wstring selfpath = GetModuleFileNameX();
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
	    it->second = load_text_file(it->second.c_str());
    } catch (const std::exception &e) {
	LOG("WARNING: %s\n", e.what());
    }
}

static
std::wstring get_output_filename(const wchar_t *ifilename, const Options &opts)
{
    if (opts.ofilename) {
	if (!std::wcscmp(opts.ofilename, L"-"))
	    return opts.ofilename;
	else
	    return GetFullPathNameX(opts.ofilename);
    }
    const wchar_t *ext =
	opts.alac_decode ? L"wav"
			 : opts.isMP4() ? L"m4a" : L"aac";
    const wchar_t *outdir = opts.outdir ? opts.outdir : L".";
    if (!std::wcscmp(ifilename, L"-"))
	return std::wstring(L"stdin.") + ext;
    else {
	std::wstring obasename =
	    PathReplaceExtension(PathFindFileNameW(ifilename), ext);
	std::wstring ofilename = 
	    GetFullPathNameX(format(L"%s/%s", outdir, obasename.c_str()));
	if (GetFullPathNameX(ifilename) == ofilename) {
	    std::wstring tl = 
		opts.isALAC() ? std::wstring(L"alac.") + ext
			      : std::wstring(L"aac.") + ext;
	    ofilename = PathReplaceExtension(ofilename, tl.c_str());
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
std::string formatSeconds(double seconds)
{
    int h, m, s, millis;
    secondsToHMS(seconds, &h, &m, &s, &millis);
    return h ? format("%d:%02d:%02d.%03d", h, m, s, millis)
	     : format("%d:%02d.%03d", m, s, millis);
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
    std::string m_message;
    bool m_verbose;
public:
    PeriodicDisplay(uint32_t interval, bool verbose=true)
	: m_interval(interval),
	  m_verbose(verbose),
	  m_last_tick(GetTickCount())
    {}
    void put(const std::string &message) {
	m_message = message;
	uint32_t tick = GetTickCount();
	if (tick - m_last_tick > m_interval) {
	    flush();
	    m_last_tick = tick;
	}
    }
    void flush() {
	if (m_verbose) std::fputs(m_message.c_str(), stderr);
	std::vector<char> s(m_message.size() + 1);
	std::strcpy(&s[0], m_message.c_str());
	squeeze(&s[0], "\r");
	SetConsoleTitleA(format(PROGNAME " %s", &s[0]).c_str());
    }
};

class Progress {
    PeriodicDisplay m_disp;
    bool m_verbose;
    uint64_t m_total;
    uint32_t m_rate;
    std::string m_tstamp;
    Timer m_timer;
public:
    Progress(bool verbosity, uint64_t total, uint32_t rate)
	: m_disp(100, verbosity), m_verbose(verbosity),
	  m_total(total), m_rate(rate)
    {
	if (total != -1)
	    m_tstamp = formatSeconds(static_cast<double>(total) / rate);
    }
    void update(uint64_t current)
    {
	double fcurrent = current;
	double percent = 100.0 * fcurrent / m_total;
	double seconds = fcurrent / m_rate;
	double ellapsed = m_timer.ellapsed();
	double eta = ellapsed * (m_total / fcurrent - 1);
	double speed = ellapsed ? seconds/ellapsed : 0.0;
	if (m_total == -1)
	    m_disp.put(format("\r%s (%.1fx)   ",
		formatSeconds(seconds).c_str(), speed));
	else
	    m_disp.put(format("\r[%.1f%%] %s/%s (%.1fx), ETA %s  ",
		percent, formatSeconds(seconds).c_str(), m_tstamp.c_str(),
		speed, formatSeconds(eta).c_str()));
    }
    void finish(uint64_t current)
    {
	m_disp.flush();
	if (m_verbose) fputc('\n', stderr);
	double ellapsed = m_timer.ellapsed();
	LOG("%" PRId64 "/%" PRId64 " samples processed in %s\n",
		current, m_total, formatSeconds(ellapsed).c_str());
    }
};

static
void do_encode(IEncoder *encoder, const std::wstring &ofilename,
	const Options &opts)
{
    typedef x::shared_ptr<std::FILE> file_t;
    file_t statfp;
    if (opts.save_stat) {
	std::wstring statname = PathReplaceExtension(ofilename, L".stat.txt");
	statfp = file_t(wfopenx(statname.c_str(), L"w"), std::fclose);
    }
    IEncoderStat *stat = dynamic_cast<IEncoderStat*>(encoder);
    Progress progress(opts.verbose, encoder->src()->length(),
	    encoder->getOutputDescription().mSampleRate);
    try {
	while (encoder->encodeChunk(1)) {
	    progress.update(stat->samplesRead());
	    if (statfp.get())
		std::fprintf(statfp.get(), "%g\n", stat->currentBitrate());
	}
	progress.finish(stat->samplesRead());
    } catch (const std::exception &e) {
	LOG("\n%s\n", e.what());
    }
}

static
x::shared_ptr<ISource> open_source(const Options &opts)
{
#ifdef NO_QT
    if (opts.alac_decode)
	return x::shared_ptr<ISource>(new ALACSource(opts.ifilename));
#endif
    StdioChannel channel(opts.ifilename);
    InputStream stream(channel);

    if (opts.is_raw) {
	SampleFormat sf(nallow(opts.raw_format).c_str(),
		opts.raw_channels, opts.raw_sample_rate);
	return x::shared_ptr<ISource>(new RawSource(stream, sf));
    }

    try {
	try {
	    return x::shared_ptr<ISource>(
		    new WaveSource(stream, opts.ignore_length));
	} catch (const std::runtime_error&) {
	    if (!stream.seekable())
		throw;
	    stream.rewind();
	}

	if (opts.libflac.loaded()) {
	    try {
		return x::shared_ptr<ISource>(
			new FLACSource(opts.libflac, stream));
	    } catch (const std::runtime_error&) {
		stream.rewind();
	    }
	}
	if (opts.libwavpack.loaded()) {
	    try {
		return x::shared_ptr<ISource>(
			new WavpackSource(opts.libwavpack, stream));
	    } catch (const std::runtime_error&) {
		stream.rewind();
	    }
	}
	if (opts.libtak.loaded() && opts.libtak.compatible()) {
	    try {
		return x::shared_ptr<ISource>(
			new TakSource(opts.libtak, stream));
	    } catch (const std::runtime_error&) {
		stream.rewind();
	    }
	}
	if (opts.libsndfile.loaded()) {
	    try {
		return x::shared_ptr<ISource>(
			new LibSndfileSource(opts.libsndfile, opts.ifilename));
	    } catch (const std::runtime_error&) {}
	}
#ifndef NO_QT
	return x::shared_ptr<ISource>(new QTMovieSource(opts.ifilename));
#endif
    } catch (const std::runtime_error&) {}
    throw std::runtime_error("Not available input file format");
}

static
void load_chapter_file(const wchar_t *chapter_file,
	std::vector<std::pair<std::wstring, int64_t> > *chapters)
{
    try {
	std::vector<std::pair<std::wstring, int64_t> > chaps;

	std::wstring str = load_text_file(chapter_file);
	std::vector<wchar_t> buf(str.begin(), str.end());
	buf.push_back(0);
	wchar_t *p = &buf[0], *tok;
	const wchar_t *tfmt = L"%02d:%02d:%02d.%03d";
	int h, m, s, ms;
	int64_t stamp = 0;
	while ((tok = wcssep(&p, L"\n"))) {
	    if (swscanf(tok, tfmt, &h, &m, &s, &ms) == 4) {
		wcssep(&tok, L"\t ");
		std::wstring name = tok ? tok : L"";
		int64_t hh = h;
		stamp = (((hh * 60) + m) * 60 + s) * 1000 + ms;
		chaps.push_back(std::make_pair(name, stamp));
	    } else if (wcsncmp(tok, L"Chapter", 7) == 0) {
		wchar_t *key = wcssep(&tok, L"=");
		if (wcsstr(key, L"NAME")) {
		    std::wstring name = tok ? tok : L"";
		    chaps.push_back(std::make_pair(name, stamp));
		} else if (swscanf(tok, tfmt, &h, &m, &s, &ms) == 4) {
		    int64_t hh = h;
		    stamp = (((hh * 60) + m) * 60 + s) * 1000 + ms;
		}
	    }
	}
	chapters->swap(chaps);
    } catch (const std::exception &e) {
	LOG("WARNING: %s\n", e.what());
    }
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
    if (parser) {
	editor.setTag(parser->getTags());
	const std::vector<std::pair<std::wstring, int64_t> > *chapters
	    = parser->getChapters();
	double rate_ratio
	    = oformat.mSampleRate / src->getSampleFormat().m_rate;
	if (chapters) {
	    if (rate_ratio == 1.0)
		editor.setChapters(*chapters);
	    else {
		std::vector<std::pair<std::wstring, int64_t> > chaps;
		std::copy(chapters->begin(), chapters->end(),
			std::back_inserter(chaps));
		for (size_t i = 0; i < chaps.size(); ++i)
		    chaps[i].second = chaps[i].second * rate_ratio + 0.5;
		editor.setChapters(chaps);
	    }
	}
    }
    if (!opts.is_raw && std::wcscmp(opts.ifilename, L"-")) {
	try {
	    editor.fetchAiffID3Tags(opts.ifilename);
	} catch (const std::exception &) {}
    }
    editor.setTag(opts.tagopts);
    editor.setTag(Tag::kTool, opts.encoder_name + L", " + encoder_config);
#ifndef NO_QT
    if (opts.isAAC()) {
	IEncoderStat *stat = dynamic_cast<IEncoderStat*>(encoder);
	GaplessInfo gi;
	double ratio = oformat.mSampleRate / iformat.mSampleRate;
	aac::CalcGaplessInfo(stat, opts.isSBR(), ratio, &gi);
	editor.setGaplessInfo(gi);
    }
#endif
    editor.setArtworkSize(opts.artwork_size);
    for (size_t i = 0; i < opts.artworks.size(); ++i)
	editor.addArtwork(opts.artworks[i].c_str());
    if (opts.chapter_file) {
	std::vector<std::pair<std::wstring, int64_t> > chaps;
	load_chapter_file(opts.chapter_file, &chaps);
	// convert from absolute millis to dulation in samples
	for (size_t i = 0; i < chaps.size() - 1; ++i) {
	    int64_t dur = chaps[i+1].second - chaps[i].second;
	    dur = dur / 1000.0 * oformat.mSampleRate + 0.5;
	    chaps[i].second = dur;
	}
	// last entry needs calculation from media length
	if (chaps.size()) {
	    IEncoderStat *stat = dynamic_cast<IEncoderStat*>(encoder);
	    size_t pos = chaps.size() - 1;
	    int64_t beg =
		chaps[pos].second / 1000.0 * oformat.mSampleRate + 0.5;
	    double ratio = oformat.mSampleRate / iformat.mSampleRate;
	    int64_t dur = stat->samplesRead() * ratio - beg;
	    chaps[pos].second = dur;
	}
	// sanity check
	size_t i = 0;
	for (i = 0; i < chaps.size(); ++i) {
	    if (chaps[i].second < 0) {
		LOG("WARNING: invalid chapter time\n");
		break;
	    }
	}
	if (i == chaps.size()) editor.setChapters(chaps);
    }
    editor.save(*mp4file);
    try {
	editor.saveArtworks(*mp4file);
    } catch (const std::exception &e) {
	LOG("WARNING: %s\n", e.what());
    }
}

static void do_optimize(MP4FileX *file, const std::wstring &dst, bool verbose)
{
    try {
	file->FinishWriteX();
	MP4FileCopy optimizer(file);
	optimizer.start(w2m(dst, utf8_codecvt_facet()).c_str());
	uint64_t total = optimizer.getTotalChunks();
	PeriodicDisplay disp(100, verbose);
	for (uint64_t i = 1; optimizer.copyNextChunk(); ++i) {
	    disp.put(format("\r%" PRId64 "/%" PRId64
		    " chunks written (optimizing)", i, total).c_str());
	}
	disp.flush();
	if (verbose) std::putc('\n', stderr);
    } catch (mp4v2::impl::Exception *e) {
	handle_mp4error(e);
    }
}

static
x::shared_ptr<ISource> do_resample(
    const x::shared_ptr<ISource> &src, const Options &opts,
    uint32_t rate)
{
    LOG("Resampling with libsoxrate\n");
    SoxResampler *resampler = new SoxResampler(opts.libsoxrate, src,
	    rate, opts.normalize);
    x::shared_ptr<ISource> new_src(resampler);
    if (!opts.normalize) return new_src;

    uint64_t n = 0, rc;
    Progress progress(opts.verbose, src->length(),
	    src->getSampleFormat().m_rate);
    while ((rc = resampler->convertSamples(4096)) > 0) {
	n += rc;
	progress.update(resampler->samplesRead());
    }
    progress.finish(resampler->samplesRead());
    if (resampler->getPeak() > 1.0) {
	LOG("Peak value %g > 1.0, gain compressed.\n",
	    resampler->getPeak());
    }
    return new_src;
}

static
void build_basic_description(const SampleFormat &format,
	AudioStreamBasicDescription *result)
{
    AudioStreamBasicDescription desc = { 0 };
    desc.mFormatID = kAudioFormatLinearPCM;
    desc.mFormatFlags = kAudioFormatFlagIsPacked;
    if (format.m_type == SampleFormat::kIsSignedInteger)
	desc.mFormatFlags |= kAudioFormatFlagIsSignedInteger;
    else if (format.m_type == SampleFormat::kIsFloat)
	desc.mFormatFlags |= kAudioFormatFlagIsFloat;
    if (format.m_endian == SampleFormat::kIsBigEndian)
	desc.mFormatFlags |= kAudioFormatFlagIsBigEndian;
    desc.mFramesPerPacket = 1;
    desc.mChannelsPerFrame = format.m_nchannels;
    desc.mSampleRate = format.m_rate;
    desc.mBitsPerChannel = format.m_bitsPerSample;
    desc.mBytesPerPacket
	= desc.mBytesPerFrame
	= format.m_nchannels * format.m_bitsPerSample >> 3;
    std::memcpy(result, &desc, sizeof desc);
}

static
x::shared_ptr<ISource> mapped_source(const x::shared_ptr<ISource> &src,
    const Options &opts, AudioChannelLayoutX *orig_layout,
    AudioChannelLayoutX *mapped_layout)
{
    uint32_t nchannels = src->getSampleFormat().m_nchannels;
    x::shared_ptr<ISource> srcx(src);
    if (opts.chanmap.size()) {
	if (opts.chanmap.size() != nchannels)
	    throw std::runtime_error(
		    "nchannels of input and --chanmap spec unmatch");
	srcx = x::shared_ptr<ISource>(new ChannelMapper(src, opts.chanmap));
    }
    const std::vector<uint32_t> *chanmap = src->getChannelMap();
    int chanmask = opts.chanmask;
    if (chanmask < 0)
	chanmask = chanmap ? GetChannelMask(*chanmap) : 0;
    if (!chanmask) chanmask = GetDefaultChannelMask(nchannels);
    AudioChannelLayoutX layout = AudioChannelLayoutX::FromBitmap(chanmask);
    *orig_layout = layout;
    if (opts.native_chanmapper)
	*mapped_layout = layout;
    else {
	int nc = layout.numChannels();
	AudioChannelLayoutX mapped(nc);
	mapped->mChannelLayoutTag = GetAACLayoutTag(layout);
	std::vector<uint32_t> chanmap;
	GetAACChannelMap(layout, nc, &chanmap);
	if (layout->mNumberChannelDescriptions == nc) {
	    AudioChannelDescription *origDesc = layout->mChannelDescriptions;
	    AudioChannelDescription *newDesc = mapped->mChannelDescriptions;
	    for (size_t i = 0; i < nc; ++i) {
		size_t n = chanmap.size() ? chanmap[i] - 1: i;
		newDesc[i].mChannelLabel = origDesc[n].mChannelLabel;
	    }
	}
	srcx = x::shared_ptr<ISource>(new ChannelMapper(srcx, chanmap));
	*mapped_layout = mapped;
    }
    return srcx;
}

#ifndef NO_QT
static
x::shared_ptr<ISink> open_sink(StdAudioComponentX &encoder,
	const std::wstring &ofilename, const Options &opts)
{
    ISink *sink;
    if (opts.is_adts)
	sink = new ADTSSink(ofilename, &encoder);
    else if (opts.isALAC()) {
	std::vector<char> cookie;
	encoder.getMagicCookie(&cookie);
	sink = new ALACSink(ofilename, cookie, !opts.no_optimize);
    } else if (opts.isAAC())
	sink = new MP4Sink(ofilename, &encoder, !opts.no_optimize);
    x::shared_ptr<ISink> sinkp(sink);
    return sinkp;
}

static
const char *get_codec_name(uint32_t codec)
{
    if (codec == 'aac ') return "AAC LC Encoder";
    else if (codec == 'aach') return "AAC HE Encoder";
    else if (codec == 'alac') return "Apple Lossless Encoder";
    else return "Unknown Encoder";
}

static
std::string get_codec_version(uint32_t codec)
{
    ComponentDescription cd = { 'aenc', codec, 'appl', 0 };
    Component component = FindNextComponent(0, &cd);
    x::shared_ptr<Ptr> name(NewEmptyHandle(), DisposeHandle);
    GetComponentInfo(component, &cd, name.get(), 0, 0);
    ComponentResult version = 
	CallComponentVersion(reinterpret_cast<ComponentInstance>(component));
    std::string namex;
    if (*name.get())
	namex = p2cstr(reinterpret_cast<StringPtr>(*name.get()));
    else
	namex = get_codec_name(codec);
    return format("%s %d.%d.%d", namex.c_str(),
	    (version>>16) & 0xffff, (version>>8) & 0xff, version & 0xff);
}

static
void list_audio_encoders()
{
    ComponentDescription cd = { 0, 0, 0, 0 };
    ComponentDescription search = cd;
    Component component = 0;
    for (Component component = 0;
	 component = FindNextComponent(component, &cd);
	 cd = search)
    {
	x::shared_ptr<Ptr> name(NewEmptyHandle(), DisposeHandle);
	GetComponentInfo(component, &cd, name.get(), 0, 0);
	ComponentResult version = 
	    CallComponentVersion(
		    reinterpret_cast<ComponentInstance>(component));
	std::string namex;
	if (*name.get())
	    namex = p2cstr(reinterpret_cast<StringPtr>(*name.get()));
	else
	    namex = format("Unknown(%s)", fourcc(cd.componentSubType).svalue);
	LOG(format("%s %s.%s %d.%d.%d\n", namex.c_str(),
	    fourcc(cd.componentType).svalue,
	    fourcc(cd.componentSubType).svalue,
		(version>>16) & 0xffff,
		(version>>8) & 0xff,
		version & 0xff).c_str());
    }
}

static
std::wstring get_encoder_config(StdAudioComponentX &scaudio)
{
    std::wstring s;
    AudioStreamBasicDescription desc;
    scaudio.getBasicDescription(&desc);
    uint32_t codec = desc.mFormatID;
    s = m2w(get_codec_version(codec));
    if (codec != 'aac ' && codec != 'aach')
	return s;

    CFArrayT<CFDictionaryRef> config;
    aac::GetCodecConfigArray(&scaudio, &config);

    CFArrayT<CFStringRef> menu;
    int value = aac::GetParameterRange(config, aac::kStrategy, &menu);
    s += format(L", %s", CF2W(menu.at(value)).c_str());
    
    try {
	value = aac::GetParameterRange(config, aac::kTVBRQuality, &menu);
	s += format(L" q%d", value);
    } catch (...) {
	value = aac::GetParameterRange(config, aac::kBitRate, &menu);
	s += format(L" %skbps", CF2W(menu.at(value)).c_str());
    }

    value = aac::GetParameterRange(config, aac::kQuality, &menu);
    s += format(L", %ls", CF2W(menu.at(value)).c_str());
    return s;
}

static
void configure_scaudio(StdAudioComponentX &scaudio, const Options &opts,
	ISource *src, AudioChannelLayout *origLayout,
	AudioChannelLayout *layout)
{
    uint32_t formatID = opts.output_format;
    const SampleFormat &iformat = src->getSampleFormat();
    int nchannels = iformat.m_nchannels;

    AudioStreamBasicDescription iasbd, oasbd = { 0 };
    build_basic_description(iformat, &iasbd);
    scaudio.setInputBasicDescription(iasbd);

    scaudio.setInputChannelLayout(origLayout);
    std::wstring inputLayout = scaudio.getInputChannelLayoutName();
    scaudio.setInputChannelLayout(layout);

    oasbd.mFormatID = formatID;
    // Set temporary dummy value to obtain ApplicableChannelLayoutTagList
    // for the format
    oasbd.mChannelsPerFrame = 2;
    scaudio.setBasicDescription(oasbd);

    uint32_t aacTag = GetAACLayoutTag(origLayout);
    // get output channel layout
    AudioChannelLayoutX olayout;
    if (opts.remix == 0xffffffff) { // --remix auto
	olayout->mChannelLayoutTag = kAudioChannelLayoutTag_AAC_5_1;
	if (nchannels < 6 || aacTag == kAudioChannelLayoutTag_AAC_6_0)
	    olayout->mChannelLayoutTag = kAudioChannelLayoutTag_Stereo;
    }
    else if (opts.remix)
	olayout->mChannelLayoutTag = opts.remix;
    else if (nchannels == 3)
	olayout->mChannelLayoutTag = kAudioChannelLayoutTag_Stereo;
    else
	olayout->mChannelLayoutTag = aacTag;

    unsigned nchannelsOut = olayout.numChannels();
    if (opts.remix && nchannels == nchannelsOut) {
	if (aacTag != olayout->mChannelLayoutTag)
	    throw std::runtime_error(
	    "Remixing to a layout with same number of channels is not allowed");
    }
    /*
    if (formatID == 'alac' && nchannelsOut != 2)
	throw std::runtime_error("Only stereo is supported for ALAC");
     */

    std::vector<UInt32> avails;
    scaudio.getApplicableChannelLayoutTagList(&avails);
    if (std::find(avails.begin(), avails.end(),
		      olayout->mChannelLayoutTag) == avails.end())
	throw std::runtime_error("Not available output channel layout");
    // Now we set real number of channels
    oasbd.mChannelsPerFrame = nchannelsOut;
    scaudio.setBasicDescription(oasbd);

    try {
	scaudio.setChannelLayout(olayout);
    } catch (...) {
	if (opts.remix)
	    throw std::runtime_error("Can't remix into the specified layout");
	else
	    throw;
    }
    std::wstring outputLayout = scaudio.getChannelLayoutName();
    LOG("%ls -> %ls\n", inputLayout.c_str(), outputLayout.c_str());

    scaudio.setRenderQuality(kQTAudioRenderQuality_Max);
}

static
void parameter_not_supported(aac::ParamType param,
	const CFArrayT<CFStringRef> &menu, size_t base=0)
{
    std::string msg = format("Specified %ls value is not supported.\n"
	"Available values are:", GetParamName(param));
    for (size_t i = base; i < menu.size(); ++i)
	msg += format("\n%d: %ls", i, CF2W(menu.at(i)).c_str());
    throw std::runtime_error(msg);
}

static
void configure_aac(StdAudioComponentX &scaudio, const Options &opts)
{
    CFArrayT<CFDictionaryRef> currentConfig;
    CFArrayT<CFStringRef> menu, limits;
    aac::GetCodecConfigArray(&scaudio, &currentConfig);
    std::vector<aac::Config> config;

    // encoding strategy
    {
	unsigned method = opts.method;
	if (opts.isSBR()) {
	    int mapping[] = { 1, 0xff, 2, 0 };
	    method = mapping[opts.method];
	}
	aac::GetParameterRange(currentConfig, aac::kStrategy, &menu);
	if (method >= menu.size())
	    parameter_not_supported(aac::kStrategy, menu);
	else {
	    aac::Config t = { aac::kStrategy, method };
	    config.push_back(t);
	}
    }
    // bitrate
    {
	if (opts.method == Options::kTVBR) {
	    aac::Config t = { aac::kTVBRQuality, opts.bitrate };
	    config.push_back(t);
	    aac::Config tt = { aac::kBitRate, 0 };
	    config.push_back(tt);
	} else  {
	    aac::GetParameterRange(currentConfig,
		    aac::kBitRate, &menu, &limits);
	    size_t index = aac::GetBitrateIndex(menu, limits, opts.bitrate);
	    aac::Config t = { aac::kBitRate, index };
	    config.push_back(t);
	}
    }
    // quality
    {
	aac::GetParameterRange(currentConfig, aac::kQuality, &menu);
	if (opts.quality >= menu.size())
	    parameter_not_supported(aac::kQuality, menu);
	else {
	    aac::Config t = { aac::kQuality, opts.quality };
	    config.push_back(t);
	}
    }
    aac::SetParameters(&scaudio, config);
}

static
void install_aach_codec()
{
    HMODULE hModule = QTLoadLibrary("QuickTimeAudioSupport.qtx");
    if (hModule) {
	ComponentRoutineProcPtr proc
	    = ProcAddress(hModule, "ACMP4AACHighEfficiencyEncoderEntry"); 
	if (proc) {
	    ComponentDescription desc = { 'aenc', 'aach', 'appl', 0 };
	    RegisterComponent(&desc, proc, 0, 0, 0, 0);
	}
    }
}

static
void override_registry()
{
    std::wstring fname = get_module_directory() + L"\\qaac.reg";
    FILE *fp;
    try {
	fp = wfopenx(fname.c_str(), L"r, ccs=UNICODE");
    } catch (...) {
	return;
    }
    LOG("Found qaac.reg, overriding registry\n");
    x::shared_ptr<FILE> fptr(fp, std::fclose);
    RegAction action;
    RegParser parser;
    try {
	parser.parse(fptr, &action);
    } catch (const std::runtime_error &e) {
	LOG("WARING: %s\n", e.what());
	return;
    }
    action.show();
    action.realize();
}

void show_available_aac_settings()
{
    std::vector<CodecSetting> settings;
    aac::GetAvailableSettings(&settings);
    for (size_t i = 0; i < settings.size(); ++i) {
	std::printf("%s %dHz %ls --",
		settings[i].m_codec == 'aac ' ? "LC" : "HE",
		settings[i].m_sample_rate,
		settings[i].m_channel_layout.c_str());
	std::vector<uint32_t> &bitrates = settings[i].m_bitrates;
	for (size_t j = 0; j < bitrates.size(); ++j) {
	    int delim = j == 0 ? ' ' : ',';
	    std::printf("%c%d", delim, bitrates[j]);
	}
	std::printf("\n");
    }
}

static
void encode_file(const x::shared_ptr<ISource> &src,
	const std::wstring &ofilename, const Options &opts)
{
    uint32_t formatID = opts.output_format;
    AudioChannelLayoutX origLayout, layout;
    x::shared_ptr<ISource> srcx =
	mapped_source(src, opts, &origLayout, &layout);

    StdAudioComponentX scaudio;
    configure_scaudio(scaudio, opts, srcx.get(), origLayout, layout);
    if (opts.isAAC())
	aac::SetupSampleRate(scaudio, opts.rate, opts.isNativeResampler());
    else if (opts.rate > 0) {
	AudioStreamBasicDescription oasbd;
	scaudio.getBasicDescription(&oasbd);
	oasbd.mSampleRate = opts.rate;
	scaudio.setBasicDescription(oasbd);
    }
    AudioStreamBasicDescription iasbd, oasbd;
    scaudio.getInputBasicDescription(&iasbd);
    scaudio.getBasicDescription(&oasbd);
    const SampleFormat &iformat = srcx->getSampleFormat();

    if (iformat.m_rate != oasbd.mSampleRate) {
	LOG("%dHz -> %gHz\n", iformat.m_rate, oasbd.mSampleRate);
	if (!opts.isNativeResampler()) {
	    srcx = do_resample(srcx, opts, oasbd.mSampleRate);
	    build_basic_description(srcx->getSampleFormat(), &iasbd);
	    scaudio.setInputBasicDescription(iasbd);
	    scaudio.setBasicDescription(oasbd);
	}
    }
    {
	QTAtomContainer ac;
	SCGetSettingsAsAtomContainer(scaudio, &ac);
	SCSetSettingsFromAtomContainer(scaudio, ac);
    }
    if (opts.isAAC()) {
	configure_aac(scaudio, opts);
    } else {
	if (iasbd.mBitsPerChannel != 16 && iasbd.mBitsPerChannel != 24)
	    LOG("WARNING: Only 16/24bit format is supported for ALAC\n");
    }
    std::wstring encoder_config = get_encoder_config(scaudio);

    LOG("%ls\n", encoder_config.c_str());

    x::shared_ptr<ISink> sink = open_sink(scaudio, ofilename, opts);
    SCAudioEncoder encoder(scaudio);
    encoder.setSource(srcx);
    encoder.setSink(sink);
    do_encode(&encoder, ofilename, opts);
    if (encoder.framesWritten()) {
	LOG("Overall bitrate: %gkbps\n", encoder.overallBitrate());

	MP4SinkBase *sinkp = dynamic_cast<MP4SinkBase*>(sink.get());
	if (sinkp) {
	    // write_tags needs original src
	    write_tags(sinkp->getFile(), opts, src.get(),
		       &encoder, encoder_config);
	    if (!opts.no_optimize)
		do_optimize(sinkp->getFile(), ofilename, opts.verbose);
	}
    }
}
#else // NO_QT
static void noop(void *) {}

static
void decode_file(const x::shared_ptr<ISource> &src,
	const std::wstring &ofilename, const Options &opts)
{
    x::shared_ptr<ISource> srcx(src);
    const SampleFormat &sf = srcx->getSampleFormat();
    std::vector<uint32_t> chanmap;
    uint32_t tag = GetALACLayoutTag(sf.m_nchannels);
    uint32_t bitmap = GetAACReversedChannelMap(tag, &chanmap);
    if (chanmap.size())
	srcx = x::shared_ptr<ISource>(new ChannelMapper(srcx, chanmap));

    x::shared_ptr<FILE> fileptr;
    if (opts.ofilename && !std::wcscmp(opts.ofilename, L"-")) {
	fileptr = x::shared_ptr<FILE>(stdout, noop);
	_setmode(1, _O_BINARY);
    } else {
	FILE *fp = wfopenx(ofilename.c_str(), L"wb");
	fileptr = x::shared_ptr<FILE>(fp, std::fclose);
    }
    WavSink sink(fileptr.get(), srcx->length(), sf, bitmap);
    Progress progress(opts.verbose, srcx->length(), sf.m_rate);
    uint32_t bpf = sf.bytesPerFrame();
    std::vector<uint8_t> buffer(4096 * bpf);
    try {
	size_t nread;
	uint64_t ntotal = 0;
	while ((nread = srcx->readSamples(&buffer[0], 4096)) > 0) {
	    ntotal += nread;
	    progress.update(ntotal);
	    sink.writeSamples(&buffer[0], nread * bpf, nread);
	}
	progress.finish(ntotal);
    } catch (const std::exception &e) {
	LOG("\n%s\n", e.what());
    }
}

static
void encode_file(const x::shared_ptr<ISource> &src,
	const std::wstring &ofilename, const Options &opts)
{
    if (opts.alac_decode) {
	decode_file(src, ofilename, opts);
	return;
    }
    AudioChannelLayoutX origLayout, layout;
    x::shared_ptr<ISource> srcx =
	mapped_source(src, opts, &origLayout, &layout);

    uint32_t otag = layout->mChannelLayoutTag;
    if (otag != kAudioChannelLayoutTag_Mono &&
	otag != kAudioChannelLayoutTag_Stereo &&
	otag != kAudioChannelLayoutTag_AAC_4_0 &&
	otag != kAudioChannelLayoutTag_AAC_5_0 &&
	otag != kAudioChannelLayoutTag_AAC_5_1 &&
	otag != kAudioChannelLayoutTag_AAC_6_1 &&
	otag != kAudioChannelLayoutTag_AAC_7_1)
	throw std::runtime_error("Not supported channel layout for ALAC");

    AudioStreamBasicDescription iasbd;
    build_basic_description(srcx->getSampleFormat(), &iasbd);

    ALACEncoderX encoder(iasbd);
    encoder.setFastMode(opts.alac_fast);
    std::vector<char> cookie;
    encoder.getMagicCookie(&cookie);

    x::shared_ptr<ISink> sink(new ALACSink(ofilename, cookie,
		!opts.no_optimize));
    encoder.setSource(srcx);
    encoder.setSink(sink);
    do_encode(&encoder, ofilename, opts);
    ALACSink *asink = dynamic_cast<ALACSink*>(sink.get());
    if (encoder.framesWritten()) {
	LOG("Overall bitrate: %gkbps\n", encoder.overallBitrate());
	write_tags(asink->getFile(), opts, src.get(), &encoder,
	    L"Apple Lossless Encoder");
	if (!opts.no_optimize)
	    do_optimize(asink->getFile(), ofilename, opts.verbose);
    }
}
#endif

const char *get_qaac_version();

static
void load_modules(Options &opts)
{
    opts.libsndfile = LibSndfileModule(L"libsndfile-1.dll");
    opts.libflac = FLACModule(L"libFLAC.dll");
    opts.libwavpack = WavpackModule(L"wavpackdll.dll");
    opts.libtak = TakModule(L"tak_deco_lib.dll");
    opts.libsoxrate = SoxResamplerModule(L"libsoxrate.dll");
}

struct FNConverter {
    wchar_t operator()(wchar_t ch) {
	if (std::wcschr(L":/\\?|<>*\"", ch))
	    return L'_';
	else
	    return ch;
    }
};

struct TagLookup {
    typedef std::map<uint32_t, std::wstring> meta_t;
    const CueTrack &track;
    const meta_t &tracktags;

    TagLookup(const CueTrack &track_, const meta_t &tags)
	: track(track_), tracktags(tags) {}

    std::wstring operator()(const std::wstring &name) {
	std::wstring namex = wslower(name);
	if (namex == L"tracknumber")
	    return widen(format("%02d", track.m_number));
	std::string skey = format("%ls", namex.c_str());
	uint32_t id = Vorbis::GetIDFromTagName(skey.c_str());
	if (id == 0) return L"";
	meta_t::const_iterator iter = tracktags.find(id);
	return iter == tracktags.end() ? L"" : iter->second;
    }
};

static
void handle_cue_sheet(Options &opts)
{
    std::wstring cuepath = opts.ifilename;
    std::wstring cuedir = L".";
    for (size_t i = 0; i < cuepath.size(); ++i)
	if (cuepath[i] == L'/') cuepath[i] = L'\\';
    const wchar_t *p = std::wcsrchr(cuepath.c_str(), L'\\');
    if (p) cuedir = cuepath.substr(0, p - cuepath.c_str());

    std::wstringbuf istream(load_text_file(opts.ifilename));
    CueSheet cue;
    cue.parse(&istream);
    typedef std::map<uint32_t, std::wstring> meta_t;
    meta_t album_tags;
    Cue::ConvertToItunesTags(cue.m_meta, &album_tags, true);
    for (size_t i = 0; i < cue.m_tracks.size(); ++i) {
	CueTrack &track = cue.m_tracks[i];
	if (!track.m_segments.size())
	    continue;

	meta_t tmp;
	Cue::ConvertToItunesTags(track.m_meta, &tmp);
	meta_t track_tags = album_tags;
	for (meta_t::iterator it = tmp.begin(); it != tmp.end(); ++it)
	    track_tags[it->first] = it->second;
	track_tags[Tag::kTrack] = widen(format("%d/%d", track.m_number,
		cue.m_tracks.size()));
	if (track_tags.find(Tag::kArtist) == track_tags.end()) {
	    if (track_tags.find(Tag::kAlbumArtist) != track_tags.end())
		track_tags[Tag::kArtist] = track_tags[Tag::kAlbumArtist];
	}

	CompositeSource *csp = new CompositeSource();
	csp->setTags(track_tags);
	x::shared_ptr<ISource> source(csp);
	std::wstring ifilename;
	x::shared_ptr<ISource> src;
	for (size_t j = 0; j < track.m_segments.size(); ++j) {
	    CueSegment &seg = track.m_segments[j];
	    if (seg.m_filename == L"__GAP__") {
		opts.ifilename = L"GAP";
		if (!src)
		    throw std::runtime_error("Pre/Postgap command before song");
		src = x::shared_ptr<ISource>(
			new NullSource(src->getSampleFormat()));
	    } else {
		ifilename = PathCombineX(cuedir, seg.m_filename);
		opts.ifilename = const_cast<wchar_t*>(ifilename.c_str());
		src = open_source(opts);
	    }
	    unsigned rate = src->getSampleFormat().m_rate;
	    int64_t begin = static_cast<int64_t>(seg.m_begin) * rate / 75;
	    int64_t end = -1;
	    if (seg.m_end != -1)
		end = static_cast<int64_t>(seg.m_end) * rate / 75 - begin;
	    IPartialSource *psrc = dynamic_cast<IPartialSource*>(src.get());
	    if (!psrc)
		throw std::runtime_error("Cannot set range this filetype");
	    psrc->setRange(begin, end);
	    csp->addSource(src);
	}
	std::wstring formatstr = opts.fname_format
	    ? opts.fname_format : L"${tracknumber}${title& }${title}";
	std::wstring ofilename =
	    process_template(formatstr, TagLookup(track, track_tags));
	ofilename = strtransform(ofilename, FNConverter()) + L".stub";
	ofilename = get_output_filename(ofilename.c_str(), opts);
	LOG("\n%ls\n", PathFindFileNameW(ofilename.c_str()));
	encode_file(src, ofilename, opts);
    }
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
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF|_CRTDBG_CHECK_ALWAYS_DF);
#endif
    Options opts;

    SetDllDirectoryW(L"");
    std::setlocale(LC_CTYPE, "");
    std::setbuf(stderr, 0);

#ifdef DEBUG_ATTACH
    FILE *fp = std::fopen("CON", "r");
    std::getc(fp);
#endif
    int result = 0;
    if (!opts.parse(argc, argv))
	return 1;
    try {
	ConsoleTitleSaver consoleTitle;

	if (opts.verbose && !opts.print_available_formats)
	    Log::instance()->enable_stderr();
	if (opts.logfilename && !opts.print_available_formats)
	    Log::instance()->enable_file(opts.logfilename);

	if (opts.nice)
	    SetPriorityClass(GetCurrentProcess(), IDLE_PRIORITY_CLASS);


	std::string encoder_name;
#ifdef NO_QT
	encoder_name = format(PROGNAME " %s", get_qaac_version());
	COMInitializer __com__; // for WIC
#else
	override_registry();
	LOG("initializing QTML...");
	QTInitializer __quicktime__;
	LOG("done\n\n");
	uint32_t qtver;
	Gestalt(gestaltQuickTime, reinterpret_cast<long*>(&qtver));
	qtver >>= 16;
	encoder_name = format(PROGNAME " %s, QuickTime %d.%d.%d",
		get_qaac_version(),
		qtver >> 8, (qtver >> 4) & 0xf, qtver & 0xf);
	if (opts.isSBR() || opts.check_only || opts.print_available_formats)
	    install_aach_codec();
#endif
	opts.encoder_name = widen(encoder_name);
	LOG("%s\n", encoder_name.c_str());

	load_modules(opts);

#ifndef NO_QT
	if (opts.check_only) {
	    if (opts.libsoxrate.loaded())
		LOG("libsoxrate %s\n", opts.libsoxrate.version_string());
	    if (opts.libsndfile.loaded())
		LOG("%s\n", opts.libsndfile.version_string());
	    if (opts.libflac.loaded())
		LOG("libFLAC %s\n", opts.libflac.VERSION_STRING);
	    if (opts.libwavpack.loaded())
		LOG("wavpackdll %s\n",
			opts.libwavpack.GetLibraryVersionString());
	    if (opts.libtak.loaded()) {
		TtakInt32 var, comp;
		opts.libtak.GetLibraryVersion(&var, &comp);
		LOG("tak_deco_lib %d.%d.%d %s\n",
			var >> 16, (var >> 8) & 0xff, var & 0xff,
			opts.libtak.compatible() ? "compatible"
			                         : "incompatible");
	    }
	    return 0;
	}
	if (opts.print_available_formats) {
	    show_available_aac_settings();
	    return 0;
	}
#endif
	mp4v2::impl::log.setVerbosity(MP4_LOG_NONE);
	//mp4v2::impl::log.setVerbosity(MP4_LOG_VERBOSE4);

	load_lyrics_file(&opts);

	while ((opts.ifilename = *argv++)) {
	    const wchar_t *name = L"<stdin>";
	    if (std::wcscmp(opts.ifilename, L"-"))
		name = PathFindFileNameW(opts.ifilename);
	    LOG("\n%ls\n", name);
	    size_t len = std::wcslen(opts.ifilename);
	    if (len > 4 && wslower(opts.ifilename + (len - 4)) == L".cue")
		handle_cue_sheet(opts);
	    else {
		std::wstring ofilename
		    = get_output_filename(opts.ifilename, opts);
		x::shared_ptr<ISource> src(open_source(opts));
		encode_file(src, ofilename, opts);
	    }
	}
    } catch (const std::exception &e) {
	if (opts.print_available_formats)
	    Log::instance()->enable_stderr();
	LOG("%s\n", e.what());
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
