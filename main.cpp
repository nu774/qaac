#include <iostream>
#include <ctime>
#include <cstdarg>
#include <clocale>
#include <algorithm>
#include <functional>
#include "strcnv.h"
#include "win32util.h"
#include <shellapi.h>
#include "itunetags.h"
#include "sink.h"
#include "rawsource.h"
#include "flacsrc.h"
#include "alacsrc.h"
#include "alacsink.h"
#include "options.h"
#include "cuesheet.h"
#include "composite.h"
#include "nullsource.h"
#include "wavsource.h"
#include "expand.h"
#include "soxdsp.h"
#include "normalize.h"
#include "logging.h"
#include "textfile.h"
#include "mixer.h"
#include "intsrc.h"
#include "scaler.h"
#include "pipedreader.h"
#include "wavsink.h"
#ifdef REFALAC
#include "alacenc.h"
#else
#include <delayimp.h>
#include "AudioCodecX.h"
#include "CoreAudioEncoder.h"
#include "afsource.h"
#include "reg.h"
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
    return m2w(ex.what(), utf8_codecvt_facet());
}

std::wstring get_module_directory()
{
    std::wstring selfpath = GetModuleFileNameX(0);
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
	    return GetFullPathNameX(opts.ofilename);
    }
    const wchar_t *ext = opts.extension();
    const wchar_t *outdir = opts.outdir ? opts.outdir : L".";
    if (!std::wcscmp(ifilename, L"-"))
	return std::wstring(L"stdin.") + ext;
    else {
	std::wstring obasename =
	    PathReplaceExtension(PathFindFileNameW(ifilename), ext);
	std::wstring ofilename = 
	    GetFullPathNameX(format(L"%s/%s", outdir, obasename.c_str()));
	if (GetFullPathNameX(ifilename) == ofilename) {
	    std::string codec_name(fourcc(opts.output_format));
	    while (codec_name.size() && codec_name.back() == ' ')
		codec_name.pop_back();
	    std::wstring tl = format(L"%hs.%s", codec_name.c_str(), ext);
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
std::wstring formatSeconds(double seconds)
{
    int h, m, s, millis;
    secondsToHMS(seconds, &h, &m, &s, &millis);
    return h ? format(L"%d:%02d:%02d.%03d", h, m, s, millis)
	     : format(L"%d:%02d.%03d", m, s, millis);
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
	    squeeze(&s[0], L"\r");
	    SetConsoleTitleW(format(L"%hs %s", PROGNAME, &s[0]).c_str());
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
	if (total != -1)
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
	if (m_total == -1)
	    m_disp.put(format(L"\r%s (%.1fx)   ",
		formatSeconds(seconds).c_str(), speed));
	else
	    m_disp.put(format(L"\r[%.1f%%] %s/%s (%.1fx), ETA %s  ",
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
	const Options &opts)
{
    typedef x::shared_ptr<std::FILE> file_t;
    file_t statPtr;
    if (opts.save_stat) {
	std::wstring statname = PathReplaceExtension(ofilename, L".stat.txt");
	statPtr = file_t(wfopenx(statname.c_str(), L"w"), std::fclose);
    }
    IEncoderStat *stat = dynamic_cast<IEncoderStat*>(encoder);
    ISource *src = encoder->src();
    if (src->length() == -1) {
	DelegatingSource *dsrc = dynamic_cast<DelegatingSource*>(src);
	while (dsrc && !dynamic_cast<PipedReader*>(src)) {
	    src = dsrc->source();
	    dsrc = dynamic_cast<DelegatingSource*>(src);
	}
    }
    Progress progress(opts.verbose, src->length(),
		      src->getSampleFormat().m_rate);
    try {
	FILE *statfp = statPtr.get();
	while (encoder->encodeChunk(1)) {
	    progress.update(src->getSamplesRead());
	    if (statfp)
		std::fwprintf(statfp, L"%g\n", stat->currentBitrate());
	}
	progress.finish(src->getSamplesRead());
    } catch (...) {
	LOG(L"\n");
	throw;
    }
}

static
x::shared_ptr<ISource> open_source(const wchar_t *ifilename,
				   const Options &opts)
{
    StdioChannel channel(ifilename);
    InputStream stream(channel);

#define MAKE_SHARED(Foo) x::shared_ptr<ISource>(new Foo)
#define TRY_MAKE_SHARED(Foo) \
    do { \
	try { return MAKE_SHARED(Foo); } \
	catch (std::exception) { stream.rewind(); } \
    } while (0) 

    if (opts.is_raw) {
	SampleFormat sf(nallow(opts.raw_format).c_str(),
			opts.raw_channels, opts.raw_sample_rate);
	return MAKE_SHARED(RawSource(stream, sf));
    }

    try {
	try {
	    return MAKE_SHARED(WaveSource(stream, opts.ignore_length));
	} catch (const std::runtime_error&) {
	    if (!stream.seekable())
		throw;
	    stream.rewind();
	}
	if (opts.libflac.loaded())
	    TRY_MAKE_SHARED(FLACSource(opts.libflac, stream));

	if (opts.libwavpack.loaded())
	    TRY_MAKE_SHARED(WavpackSource(opts.libwavpack, stream,
					  ifilename));

	if (opts.libtak.loaded() && opts.libtak.compatible())
	    TRY_MAKE_SHARED(TakSource(opts.libtak, stream));
#ifndef REFALAC
	try {
	    return AudioFileOpenFactory(stream, ifilename);
	} catch (...) {
	    stream.rewind();
	}
#endif
	if (opts.libsndfile.loaded())
	    TRY_MAKE_SHARED(LibSndfileSource(opts.libsndfile, ifilename));

#ifdef REFALAC
	return x::shared_ptr<ISource>(new ALACSource(ifilename));
#endif
    } catch (const std::runtime_error&) {}
    throw std::runtime_error("Not available input file format");
#undef TRY_MAKE_SHARED
#undef MAKE_SHARED
}

static
void load_chapter_file(const Options &opts,
	std::vector<std::pair<std::wstring, int64_t> > *chapters)
{
    try {
	const wchar_t *chapter_file = opts.chapter_file;
	std::vector<std::pair<std::wstring, int64_t> > chaps;

	std::wstring str = load_text_file(chapter_file, opts.textcp);
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
	LOG(L"WARNING: %s\n", errormsg(e).c_str());
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

    double oSampleRate = oformat.mSampleRate;
    if (opts.isSBR()) oSampleRate /= 2;

    ITagParser *parser = dynamic_cast<ITagParser*>(src);
    IEncoderStat *stat = dynamic_cast<IEncoderStat*>(encoder);

    if (parser) {
	editor.setTag(parser->getTags());
	const std::vector<std::pair<std::wstring, int64_t> > *chapters
	    = parser->getChapters();
	double rate_ratio = oSampleRate / src->getSampleFormat().m_rate;
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
    editor.setTag(opts.tagopts);
    editor.setTag(Tag::kTool, opts.encoder_name + L", " + encoder_config);
#ifndef REFALAC
    if (opts.isAAC() && stat->samplesWritten()) {
	AudioConverterX converter =
	    dynamic_cast<CoreAudioEncoder*>(encoder)->getConverter();
	AudioConverterPrimeInfo info;
	converter.getPrimeInfo(&info);
	GaplessInfo gi;
	gi.delay = info.leadingFrames;
	gi.padding = info.trailingFrames;
	uint64_t nsamples = stat->samplesWritten();
	if (opts.isSBR()) nsamples /= 2;
	gi.samples = nsamples - gi.delay - gi.padding;
	editor.setGaplessInfo(gi);
    }
#endif
    editor.setArtworkSize(opts.artwork_size);
    for (size_t i = 0; i < opts.artworks.size(); ++i)
	editor.addArtwork(opts.artworks[i].c_str());
    if (opts.chapter_file) {
	std::vector<std::pair<std::wstring, int64_t> > chaps;
	load_chapter_file(opts, &chaps);
	if (chaps.size()) {
	    size_t i = 0;
	    // convert from absolute millis to dulation in samples
	    for (i = 0; i < chaps.size() - 1; ++i) {
		int64_t dur = chaps[i+1].second - chaps[i].second;
		dur = dur / 1000.0 * oSampleRate + 0.5;
		chaps[i].second = dur;
	    }
	    // last entry needs calculation from media length
	    IEncoderStat *stat = dynamic_cast<IEncoderStat*>(encoder);
	    size_t pos = chaps.size() - 1;
	    int64_t beg = chaps[pos].second / 1000.0 * oSampleRate + 0.5;
	    double ratio = oSampleRate / iformat.mSampleRate;
	    int64_t dur = stat->samplesRead() * ratio - beg;
	    chaps[pos].second = dur;
	    // sanity check
	    for (i = 0; i < chaps.size(); ++i) {
		if (chaps[i].second < 0) {
		    LOG(L"WARNING: invalid chapter time\n");
		    break;
		}
	    }
	    if (i == chaps.size()) editor.setChapters(chaps);
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
	optimizer.start(w2m(dst, utf8_codecvt_facet()).c_str());
	uint64_t total = optimizer.getTotalChunks();
	PeriodicDisplay disp(100, verbose);
	for (uint64_t i = 1; optimizer.copyNextChunk(); ++i) {
	    disp.put(format(L"\r%lld/%lld chunks written (optimizing)",
			    i, total).c_str());
	}
	disp.flush();
	if (verbose) std::putwc(L'\n', stderr);
    } catch (mp4v2::impl::Exception *e) {
	handle_mp4error(e);
    }
}

static
x::shared_ptr<ISource> do_normalize(
    const x::shared_ptr<ISource> &src, const Options &opts)
{
    Normalizer *normalizer = new Normalizer(src);
    x::shared_ptr<ISource> new_src(normalizer);

    LOG(L"Scanning maximum peak...\n");
    uint64_t n = 0, rc;
    Progress progress(opts.verbose, src->length(),
	    src->getSampleFormat().m_rate);
    while ((rc = normalizer->process(4096)) > 0) {
	n += rc;
	progress.update(normalizer->samplesRead());
    }
    progress.finish(normalizer->samplesRead());
    LOG(L"Peak value: %g\n", normalizer->getPeak());
    return new_src;
}

static
void build_basic_description(const SampleFormat &format,
	AudioStreamBasicDescription *result)
{
    AudioStreamBasicDescription desc = { 0 };
    desc.mFormatID = 'lpcm';
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
FILE *open_config_file(const wchar_t *file)
{
    std::vector<std::wstring> search_paths;
    const wchar_t *home = _wgetenv(L"HOME");
    if (home)
	search_paths.push_back(format(L"%s\\%s", home, L".qaac"));
    wchar_t path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathW(0, CSIDL_APPDATA, 0, 0, path)))
	search_paths.push_back(format(L"%s\\%s", path, L"qaac"));
    search_paths.push_back(get_module_directory());
    for (size_t i = 0; i < search_paths.size(); ++i) {
	try {
	    std::wstring pathtry =
		format(L"%s\\%s", search_paths[i].c_str(), file);
	    return wfopenx(pathtry.c_str(), L"r");
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
    FILE *fp;
    if (opts.remix_preset) {
	std::wstring path = format(L"matrix\\%s.txt", opts.remix_preset);
	fp = open_config_file(path.c_str());
    } else
	fp = wfopenx(opts.remix_file, L"r");
    x::shared_ptr<FILE> fpPtr(fp, std::fclose);
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

static
x::shared_ptr<ISource> mapped_source(const x::shared_ptr<ISource> &src,
    const Options &opts, uint32_t *wav_chanmask,
    AudioChannelLayoutX *aac_layout, bool threading)
{
    uint32_t nchannels = src->getSampleFormat().m_nchannels;
    x::shared_ptr<ISource> srcx(src);

    const std::vector<uint32_t> *channels = src->getChannels();
    if (channels) {
	if (opts.verbose > 1) {
	    LOG(L"Input layout: %hs\n",
		chanmap::GetChannelNames(*channels).c_str());
	}
	// reorder to Microsoft (USB) order
	std::vector<uint32_t> work;
	chanmap::ConvertChannelsFromAppleLayout(*channels, &work);
	std::vector<uint32_t> mapping;
	chanmap::GetChannelMappingToUSBOrder(work, &mapping);
	srcx.reset(new ChannelMapper(srcx, mapping,
				     chanmap::GetChannelMask(work)));
	channels = srcx->getChannels();
    }
    // remix
    if (opts.remix_preset || opts.remix_file) {
	if (!opts.libsoxrate.loaded())
	    LOG(L"WARNING: mixer requires libsoxrate. Mixing disabled\n");
	else {
	    std::vector<std::vector<complex_t> > matrix;
	    matrix_from_preset(opts, &matrix);
	    if (opts.verbose > 1 || opts.logfilename) {
		LOG(L"Matrix mixer: %dch -> %dch\n",
		    matrix[0].size(), matrix.size());
	    }
	    srcx.reset(new MatrixMixer(srcx, opts.libsoxrate,
				       matrix, threading));
	    channels = 0;
	    nchannels = srcx->getSampleFormat().m_nchannels;
	}
    }
    // map with --chanmap option
    if (opts.chanmap.size()) {
	if (opts.chanmap.size() != nchannels)
	    throw std::runtime_error(
		    "nchannels of input and --chanmap spec unmatch");
	srcx.reset(new ChannelMapper(srcx, opts.chanmap));
    }
    // retrieve original channel layout, taking --chanmask into account
    if (opts.chanmask > 0 && bitcount(opts.chanmask) != nchannels)
	throw std::runtime_error("unmatch number of channels with --chanmask");
    int chanmask = opts.chanmask;
    if (chanmask < 0)
	chanmask = channels ? chanmap::GetChannelMask(*channels) : 0;
    if (!chanmask && !opts.isLPCM()) {
	if (opts.verbose >1 || opts.logfilename)
	    LOG(L"Using default channel layout.\n");
	chanmask = chanmap::GetDefaultChannelMask(nchannels);
    }
    AudioChannelLayoutX layout;
    *wav_chanmask = chanmask;
    if (chanmask) {
	if (opts.isLPCM() && opts.verbose > 1) {
	    std::vector<uint32_t> vec;
	    chanmap::GetChannels(chanmask, &vec);
	    LOG(L"Output layout: %hs\n",
		chanmap::GetChannelNames(vec).c_str());
	}
    }
    if (!opts.isLPCM()) {
	// construct mapped channel layout to AAC/ALAC order
	int nc = layout.numChannels();
	AudioChannelLayoutX mapped;
	mapped->mChannelLayoutTag = chanmap::GetAACLayoutTag(chanmask);
	std::vector<uint32_t> aacmap;
	chanmap::GetAACChannelMap(chanmask, &aacmap);
	if (aacmap.size())
	    srcx = x::shared_ptr<ISource>(new ChannelMapper(srcx, aacmap));
	*aac_layout = mapped;
	if (opts.verbose > 1) {
	    std::vector<uint32_t> vec;
	    chanmap::GetChannels(mapped, &vec);
	    LOG(L"Output layout: %hs\n",
		chanmap::GetChannelNames(vec).c_str());
	}
    }
    return srcx;
}

static
x::shared_ptr<ISource> delayed_source(const x::shared_ptr<ISource> &src,
				      const Options & opts)
{
    double rate = src->getSampleFormat().m_rate;
    if (opts.delay > 0) {
	CompositeSource *cp = new CompositeSource();
	x::shared_ptr<ISource> cpPtr(cp);
	NullSource *ns = new NullSource(src->getSampleFormat());
	int nsamples = lrint(0.001 * opts.delay * rate);
	ns->setRange(0, nsamples);
	cp->addSource(x::shared_ptr<ISource>(ns));
	cp->addSource(src);
	if (opts.verbose > 1 || opts.logfilename)
	    LOG(L"Delay of %dms: pad %d samples\n", opts.delay,
		nsamples);
	return cpPtr;
    } else if (opts.delay < 0) {
	IPartialSource *p = dynamic_cast<IPartialSource*>(src.get());
	if (p) {
	    int nsamples = lrint(-0.001 * opts.delay * rate);
	    if (src->length() >= 0 && nsamples > src->length())
		nsamples = src->length();
	    p->setRange(nsamples, -1);
	    if (opts.verbose > 1 || opts.logfilename)
		LOG(L"Delay of %dms: truncate %d samples\n", opts.delay,
		    nsamples);
	}
	else
	    LOG(L"WARNING: can't set negative delay for this input\n");
    }
    return src;
}

#ifndef REFALAC
static
void config_aac_codec(AudioConverterX &converter, const Options &opts);
#endif

static x::shared_ptr<ISource> 
preprocess_input(const x::shared_ptr<ISource> &src,
		 const Options &opts, uint32_t *wChanmask,
		 AudioChannelLayoutX *aLayout,
		 AudioStreamBasicDescription *inputDesc,
		 AudioStreamBasicDescription *outputDesc)
{
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    bool threading = opts.threading && si.dwNumberOfProcessors > 1;
#ifndef REFALAC
    x::shared_ptr<AudioCodecX> codec;
    if (!opts.isLPCM())
	codec.reset(new AudioCodecX(opts.output_format));
#endif
    x::shared_ptr<ISource> srcx = delayed_source(src, opts);

    uint32_t wavChanmask;
    AudioChannelLayoutX aacLayout;
    srcx = mapped_source(srcx, opts, &wavChanmask, &aacLayout, threading);
    if (!opts.isLPCM()) {
#ifndef REFALAC
	if (!codec->isAvailableOutputChannelLayout(
					   aacLayout->mChannelLayoutTag))
	    throw std::runtime_error("Channel layout not supported");
#else
	switch (aacLayout->mChannelLayoutTag) {
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
    AudioStreamBasicDescription iasbd;
    build_basic_description(srcx->getSampleFormat(), &iasbd);

    AudioStreamBasicDescription oasbd = { 0 };
    oasbd.mFormatID = opts.output_format;
    oasbd.mChannelsPerFrame = aacLayout.numChannels();

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
	    AudioConverterX converter(iasbd, oasbd);
	    converter.setInputChannelLayout(aacLayout);
	    converter.setOutputChannelLayout(aacLayout);
	    config_aac_codec(converter, opts);
	    converter.getOutputStreamDescription(&oasbd);
	}
    }
#endif
    if (oasbd.mSampleRate != iasbd.mSampleRate) {
	LOG(L"%gHz -> %gHz\n", iasbd.mSampleRate, oasbd.mSampleRate);
	if (!opts.native_resampler && opts.libsoxrate.loaded()) {
	    x::shared_ptr<ISoxDSPEngine>
		engine(new SoxResampler(opts.libsoxrate,
					srcx->getSampleFormat(),
					oasbd.mSampleRate,
					threading));
	    srcx.reset(new SoxDSPProcessor(engine, srcx));
	}
    }
    if (opts.lowpass > 0) {
	if (!opts.libsoxrate.loaded())
	    LOG(L"WARNING: --lowpass requires libsoxrate. LPF disabled\n");
        else {
	    if (opts.verbose > 1 || opts.logfilename)
		LOG(L"Applying LPF: %dHz\n", opts.lowpass);
	    x::shared_ptr<ISoxDSPEngine>
		engine(new SoxLowpassFilter(opts.libsoxrate,
					    srcx->getSampleFormat(),
					    opts.lowpass,
					    threading));
	    srcx.reset(new SoxDSPProcessor(engine, srcx));
	}
    }
    if (opts.normalize)
	srcx = do_normalize(srcx, opts);
    if (opts.gain) {
	double scale = dB_to_scale(opts.gain);
	if (opts.verbose > 1 || opts.logfilename)
	    LOG(L"Gain adjustment: %gdB, scale factor %g\n",
		opts.gain, scale);
	srcx.reset(new Scaler(srcx, scale));
    }
    if (opts.bits_per_sample) {
	if (opts.isAAC())
	    LOG(L"WARNING: --bits-per-sample has no effect for AAC\n");
	else if (srcx->getSampleFormat().m_bitsPerSample
		 != opts.bits_per_sample) {
	    srcx.reset(new IntegerSource(srcx, opts.bits_per_sample));
	    if (opts.verbose > 1 || opts.logfilename)
		LOG(L"Convert to %hs\n",
		    srcx->getSampleFormat().str().c_str());
	}
    }
    if (opts.output_format == 'alac') {
	const SampleFormat &sf = srcx->getSampleFormat();
	uint32_t bits = sf.m_bitsPerSample;
	if (sf.m_type != SampleFormat::kIsSignedInteger
	    || bits != 16 && bits != 24 && bits != 32) {
	    throw std::runtime_error(format(
		"Not supported sample format for ALAC: %s", sf.str().c_str()));
	}
    }
    if (threading && !opts.isLPCM()) {
	PipedReader *reader = new PipedReader(srcx);
	reader->start();
	srcx.reset(reader);
	if (opts.verbose > 1 || opts.logfilename)
	    LOG(L"Enable threading\n");
    }
    build_basic_description(srcx->getSampleFormat(), &iasbd);
    if (wChanmask) *wChanmask = wavChanmask;
    if (aLayout) *aLayout = aacLayout;
    if (inputDesc) *inputDesc = iasbd;
    if (outputDesc) *outputDesc = oasbd;
    return srcx;
}

static
void decode_file(const x::shared_ptr<ISource> &src,
		 const std::wstring &ofilename, const Options &opts,
		 uint32_t chanmask)
{
    struct F { static void close(FILE *) {} };

    x::shared_ptr<FILE> fileptr;
    const wchar_t *spath = ofilename.c_str();
    if (ofilename == L"-")
	fileptr.reset(stdout, F::close);
    else
	fileptr.reset(wfopenx(spath, L"wb"), std::fclose);

    const SampleFormat &sf = src->getSampleFormat();
    WaveSink sink(fileptr.get(), src->length(), sf, chanmask);

    ISource *srcp = src.get();
    if (srcp->length() == -1) {
	DelegatingSource *dsrc = dynamic_cast<DelegatingSource*>(srcp);
	while (dsrc && !dynamic_cast<PipedReader*>(srcp)) {
	    srcp = dsrc->source();
	    dsrc = dynamic_cast<DelegatingSource*>(srcp);
	}
    }
    Progress progress(opts.verbose, srcp->length(),
		      srcp->getSampleFormat().m_rate);
    uint32_t bpf = sf.bytesPerFrame();
    std::vector<uint8_t> buffer(4096 * bpf);
    try {
	size_t nread;
	while ((nread = src->readSamples(&buffer[0], 4096)) > 0) {
	    progress.update(srcp->getSamplesRead());
	    sink.writeSamples(&buffer[0], nread * bpf, nread);
	}
	progress.finish(srcp->getSamplesRead());
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
    s += format(L", %s", strategies[value]);
    if (value == kAudioCodecBitRateControlMode_Variable) {
	value = converter.getSoundQualityForVBR();
	s += format(L" q%d", value);
    } else {
	value = converter.getEncodeBitRate();
	s += format(L" %gkbps", value / 1000.0);
    }
    value = converter.getCodecQuality();
    s += format(L", Quality %d", value);
    return s;
}

static
x::shared_ptr<ISink> open_sink(const std::wstring &ofilename,
	const Options &opts, const std::vector<uint8_t> &cookie)
{
    ISink *sink;
    if (opts.is_adts)
	sink = new ADTSSink(ofilename, cookie);
    else if (opts.isALAC())
	sink = new ALACSink(ofilename, cookie, !opts.no_optimize);
    else if (opts.isAAC())
	sink = new MP4Sink(ofilename, cookie, !opts.no_optimize);
    return x::shared_ptr<ISink>(sink);
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
	    AudioChannelLayoutX acl;
	    acl->mChannelLayoutTag = tags[j];
	    std::vector<uint32_t> channels;
	    chanmap::GetChannels(acl, &channels);
	    std::string name = chanmap::GetChannelNames(channels);

	    AudioStreamBasicDescription iasbd = { 0 };
	    iasbd.mFormatID = 'lpcm';
	    iasbd.mFormatFlags =
		kAudioFormatFlagIsPacked | kAudioFormatFlagIsFloat;
	    iasbd.mFramesPerPacket = 1;
	    iasbd.mChannelsPerFrame = acl.numChannels();
	    iasbd.mSampleRate = srates[i].mMinimum;
	    iasbd.mBitsPerChannel = 32;
	    iasbd.mBytesPerPacket = iasbd.mBytesPerFrame
		= iasbd.mChannelsPerFrame * iasbd.mBitsPerChannel >> 3;
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
    std::wstring dllpath = GetModuleFileNameX(hDll);
    DWORD dwhandle = 0;
    DWORD size = GetFileVersionInfoSizeW(dllpath.c_str(), &dwhandle);
    std::vector<BYTE> vec(size * 4);
    GetFileVersionInfoW(dllpath.c_str(), dwhandle, vec.size(), &vec[0]);
    UINT len = 0;
    VS_FIXEDFILEINFO *vsfi;
    VerQueryValueW(&vec[0], L"\\", (void**)(&vsfi), &len);
    WORD vers[4];
    vers[0] = HIWORD(vsfi->dwFileVersionMS);
    vers[1] = LOWORD(vsfi->dwFileVersionMS);
    vers[2] = HIWORD(vsfi->dwFileVersionLS);
    vers[3] = LOWORD(vsfi->dwFileVersionLS);
    return format("%d.%d.%d.%d", vers[0], vers[1], vers[2], vers[3]);
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
    throw_win32_error(pdli->szDll, pdli->dwLastError);
    return 0;
}

static
void override_registry(int verbose)
{
    std::wstring fname = get_module_directory() + L"qaac.reg";
    FILE *fp;
    try {
	fp = wfopenx(fname.c_str(), L"r, ccs=UNICODE");
    } catch (...) {
	return;
    }
    if (verbose > 1)
	LOG(L"Found qaac.reg, overriding registry\n");
    x::shared_ptr<FILE> fptr(fp, std::fclose);
    RegAction action;
    RegParser parser;
    try {
	parser.parse(fptr, &action);
    } catch (const std::exception &e) {
	LOG(L"WARNING: %s\n", errormsg(e).c_str());
	return;
    }
    if (verbose > 1)
	action.show();
    action.realize();
}

inline
void throwIfError(HRESULT expr, const char *msg)
{
    if (FAILED(expr))
	throw_win32_error(msg, expr);
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

    override_registry(verbose);
    try {
	HKEY hKey;
	const wchar_t *subkey =
	    L"SOFTWARE\\Apple Inc.\\Apple Application Support";
	HR(RegOpenKeyExW(HKEY_LOCAL_MACHINE, subkey, 0, KEY_READ, &hKey));
	x::shared_ptr<HKEY__> hKeyPtr(hKey, RegCloseKey);
	DWORD size;
	HR(RegQueryValueExW(hKey, L"InstallDir", 0, 0, 0, &size));
	std::vector<wchar_t> vec(size/sizeof(wchar_t));
	HR(RegQueryValueExW(hKey, L"InstallDir", 0, 0,
		    reinterpret_cast<LPBYTE>(&vec[0]), &size));
	searchPaths = format(L"%s;%s", &vec[0], searchPaths.c_str());
    } catch (const std::exception &) {}
    std::wstring dir = get_module_directory() + L"QTfiles";
    searchPaths = format(L"%s;%s", dir.c_str(), searchPaths.c_str());
    SetEnvironmentVariableW(L"PATH", searchPaths.c_str());
}

inline uint32_t bound_quality(uint32_t n)
{
    return std::min(n, static_cast<uint32_t>(kAudioConverterQuality_Max));
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
void encode_file(const x::shared_ptr<ISource> &src,
	const std::wstring &ofilename, const Options &opts)
{
    uint32_t wavChanmask;
    AudioChannelLayoutX aacLayout;
    AudioStreamBasicDescription iasbd, oasbd;

    x::shared_ptr<ISource> srcx =
	preprocess_input(src, opts, &wavChanmask, &aacLayout, &iasbd, &oasbd);

    if (opts.isLPCM()) {
	decode_file(srcx, ofilename, opts, wavChanmask);
	return;
    }
    AudioConverterX converter(iasbd, oasbd);
    converter.setInputChannelLayout(aacLayout);
    converter.setOutputChannelLayout(aacLayout);
    if (opts.isAAC()) config_aac_codec(converter, opts);

    std::wstring encoder_config = get_encoder_config(converter);
    LOG(L"%s\n", encoder_config.c_str());
    std::vector<uint8_t> cookie;
    converter.getCompressionMagicCookie(&cookie);
    CoreAudioEncoder encoder(converter);
    encoder.setSource(srcx);
    x::shared_ptr<ISink> sink = open_sink(ofilename, opts, cookie);
    encoder.setSink(sink);
    do_encode(&encoder, ofilename, opts);
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
void encode_file(const x::shared_ptr<ISource> &src,
	const std::wstring &ofilename, const Options &opts)
{
    uint32_t wavChanmask;
    AudioChannelLayoutX aacLayout;
    AudioStreamBasicDescription iasbd;
    x::shared_ptr<ISource> srcx =
	preprocess_input(src, opts, &wavChanmask, &aacLayout, &iasbd, 0);
    if (opts.isLPCM()) {
	decode_file(srcx, ofilename, opts, wavChanmask);
	return;
    }
    ALACEncoderX encoder(iasbd);
    encoder.setFastMode(opts.alac_fast);
    std::vector<uint8_t> cookie;
    encoder.getMagicCookie(&cookie);

    x::shared_ptr<ISink> sink(new ALACSink(ofilename, cookie,
					   !opts.no_optimize));
    encoder.setSource(srcx);
    encoder.setSink(sink);
    do_encode(&encoder, ofilename, opts);
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
void load_modules(Options &opts)
{
    opts.libsndfile = LibSndfileModule(L"libsndfile-1.dll");
    opts.libflac = FLACModule(L"libFLAC.dll");
    opts.libwavpack = WavpackModule(L"wavpackdll.dll");
    opts.libtak = TakModule(L"tak_deco_lib.dll");
#ifdef _WIN64
    opts.libsoxrate = SoxModule(L"libsoxrate64.dll");
#else
    opts.libsoxrate = SoxModule(L"libsoxrate.dll");
#endif
}

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

static inline
uint64_t cue_frame_to_sample(uint32_t sampling_rate, uint32_t nframe)
{
    return static_cast<uint64_t>(nframe / 75.0 * sampling_rate + 0.5);
}

static
void handle_cue_sheet(const wchar_t *ifilename, const Options &opts,
		      x::shared_ptr<ISource> *result=0)
{
    std::wstring cuepath = ifilename;
    std::wstring cuedir = L".";
    for (size_t i = 0; i < cuepath.size(); ++i)
	if (cuepath[i] == L'/') cuepath[i] = L'\\';
    const wchar_t *p = std::wcsrchr(cuepath.c_str(), L'\\');
    if (p) cuedir = cuepath.substr(0, p - cuepath.c_str());

    std::wstring cuetext = load_text_file(ifilename, opts.textcp);
    std::wstringbuf istream(cuetext);
    CueSheet cue;
    cue.parse(&istream);
    typedef std::map<uint32_t, std::wstring> meta_t;
    meta_t album_tags;
    Cue::ConvertToItunesTags(cue.m_meta, &album_tags, true);

    CompositeSource *concat_sp = new CompositeSource();
    x::shared_ptr<ISource> concat_spPtr(concat_sp);
    std::vector<std::pair<std::wstring, int64_t> > chapters;

    for (size_t i = 0; i < cue.m_tracks.size(); ++i) {
	CueTrack &track = cue.m_tracks[i];
	meta_t track_tags = album_tags;
	{
	    meta_t tmp;
	    Cue::ConvertToItunesTags(track.m_meta, &tmp);
	    for (meta_t::iterator it = tmp.begin(); it != tmp.end(); ++it)
		track_tags[it->first] = it->second;
	    track_tags[Tag::kTrack] =
		widen(format("%d/%d", track.m_number,
			     cue.m_tracks.back().m_number));
	}
	CompositeSource *csp = new CompositeSource();
	x::shared_ptr<ISource> csPtr(csp);
	csp->setTags(track_tags);
	x::shared_ptr<ISource> src;
	for (size_t j = 0; j < track.m_segments.size(); ++j) {
	    CueSegment &seg = track.m_segments[j];
	    if (seg.m_filename == L"__GAP__") {
		if (!src.get()) continue;
		src.reset(new NullSource(src->getSampleFormat()));
	    } else {
		std::wstring ifilename = PathCombineX(cuedir, seg.m_filename);
		src = open_source(ifilename.c_str(), opts);
	    }
	    unsigned rate = src->getSampleFormat().m_rate;
	    int64_t begin = cue_frame_to_sample(rate, seg.m_begin);
	    int64_t end = seg.m_end == -1 ? -1:
		cue_frame_to_sample(rate, seg.m_end) - begin;
	    IPartialSource *psrc = dynamic_cast<IPartialSource*>(src.get());
	    if (!psrc)
		throw std::runtime_error("Cannot set range this filetype");
	    psrc->setRange(begin, end);
	    csp->addSource(src);
	}
	if (opts.concat_cue) {
	    concat_sp->addSource(csPtr);
	    chapters.push_back(std::make_pair(track.getName(),
					      csPtr->length()));
	} else {
	    std::wstring formatstr = opts.fname_format
		? opts.fname_format : L"${tracknumber}${title& }${title}";
	    std::wstring ofilename =
		process_template(formatstr, TagLookup(track, track_tags));

	    struct F {
		static wchar_t trans(wchar_t ch) {
		    return std::wcschr(L":/\\?|<>*\"", ch) ? L'_' : ch;
		}
	    };
	    ofilename = strtransform(ofilename, F::trans) + L".stub";
	    ofilename = get_output_filename(ofilename.c_str(), opts);
	    LOG(L"\n%s\n", PathFindFileNameW(ofilename.c_str()));
	    encode_file(csPtr, ofilename, opts);
	}
    }
    if (opts.concat_cue) {
	concat_sp->setTags(album_tags);
	concat_sp->setChapters(chapters);
	if (opts.concat)
	    result->swap(concat_spPtr);
	else {
	    std::wstring ofilename = get_output_filename(ifilename, opts);
	    encode_file(concat_spPtr, ofilename, opts);
	}
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
#ifdef _MSC_VER
    _setmode(1, _O_U8TEXT);
    _setmode(2, _O_U8TEXT);
#endif

#ifdef DEBUG_ATTACH
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
	encoder_name = format(PROGNAME " %s", get_qaac_version());
#ifndef REFALAC
	__pfnDliFailureHook2 = DllImportHook;
	set_dll_directories(opts.verbose);
	HMODULE hDll = LoadLibraryW(L"CoreAudioToolbox.dll");
	if (!hDll)
	    throw_win32_error("CoreAudioToolbox.dll", GetLastError());
	else {
	    std::string ver = GetCoreAudioVersion(hDll);
	    encoder_name = format("%s, CoreAudioToolbox %s",
		    encoder_name.c_str(), ver.c_str());
	    setup_aach_codec(hDll);
	    FreeLibrary(hDll);
	}
#endif
	opts.encoder_name = widen(encoder_name);
	if (!opts.print_available_formats)
	    LOG(L"%s\n", opts.encoder_name.c_str());

	load_modules(opts);

	if (opts.check_only) {
	    if (opts.libsoxrate.loaded())
		LOG(L"libsoxrate %hs\n",
		    opts.libsoxrate.version_string());
	    if (opts.libsndfile.loaded())
		LOG(L"%hs\n", opts.libsndfile.version_string());
	    if (opts.libflac.loaded())
		LOG(L"libFLAC %hs\n", opts.libflac.VERSION_STRING);
	    if (opts.libwavpack.loaded())
		LOG(L"wavpackdll %hs\n",
		    opts.libwavpack.GetLibraryVersionString());
	    if (opts.libtak.loaded()) {
		TtakInt32 var, comp;
		opts.libtak.GetLibraryVersion(&var, &comp);
		LOG(L"tak_deco_lib %d.%d.%d %hs\n",
			var >> 16, (var >> 8) & 0xff, var & 0xff,
			opts.libtak.compatible() ? "compatible"
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
	    env += GetFullPathNameX(opts.tmpdir);
	    _wputenv(env.c_str());
	}

	if (opts.ofilename) {
	    std::wstring fullpath = GetFullPathNameX(opts.ofilename);
	    const wchar_t *ws = fullpath.c_str();
	    if (!std::wcscmp(opts.ofilename, L"-"))
		_setmode(1, _O_BINARY);
	    else if (std::wcsstr(ws, L"\\\\.\\pipe\\") == ws) {
		if (opts.isMP4())
		    throw std::runtime_error("MP4 piping is not supported");
		opts.ofilename = L"-";
		_dup2(win32_create_named_pipe(ws), 1);
		_setmode(1, _O_BINARY);
	    }
	}
	CompositeSource *csp = new CompositeSource();
	x::shared_ptr<ISource> csPtr(csp);
	const wchar_t *ifilename = 0;
	for (int i = 0; i < argc; ++i) {
	    ifilename = argv[i];
	    x::shared_ptr<ISource> src;
	    const wchar_t *name = L"<stdin>";
	    if (std::wcscmp(ifilename, L"-"))
		name = PathFindFileNameW(ifilename);
	    if (!opts.concat) LOG(L"\n%s\n", name);
	    if (wslower(PathFindExtension(ifilename)) == L".cue")
		handle_cue_sheet(ifilename, opts, &src);
	    else {
		std::wstring ofilename
		    = get_output_filename(ifilename, opts);
		src = open_source(ifilename, opts);
		if (!opts.concat) encode_file(src, ofilename, opts);
	    }
	    if (opts.concat) csp->addSourceWithChapter(src);
	}
	if (opts.concat) {
	    std::wstring ofilename = get_output_filename(ifilename, opts);
	    LOG(L"\n%s\n", PathFindFileNameW(ofilename.c_str()));
	    encode_file(csPtr, ofilename, opts);
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
