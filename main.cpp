#include <iostream>
#include <ctime>
#include <cstdarg>
#include <clocale>
#include <algorithm>
#include <functional>
#include <QTML.h>
#include "strcnv.h"
#include "win32util.h"
#include <shellapi.h>
#include "aacencoder.h"
#include "itunetags.h"
#include "sink.h"
#include "rawsource.h"
#include "libsndfilesrc.h"
#include "flacsrc.h"
#include "wvpacksrc.h"
#include "qtmoviesource.h"
#include "alacsink.h"
#include "options.h"
#include "cuesheet.h"
#include "composite.h"
#include "nullsource.h"
#include "wavsource.h"
#include "expand.h"
#include "resampler.h"
#include "logging.h"
#include "reg.h"

#include <crtdbg.h>

class PeriodicDisplay {
    uint32_t m_interval;
    uint32_t m_last_tick;
    std::string m_message;
    bool m_verbose;
public:
    PeriodicDisplay(uint32_t interval, bool verbose=true)
	: m_interval(interval),
	  m_verbose(verbose),
	  m_last_tick(0)
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
	SetConsoleTitleA(format("qaac %s", &s[0]).c_str());
    }
};

static
void parameter_not_supported(const wchar_t *name,
	const CFArrayT<CFStringRef> &menu, size_t base=0)
{
    LOG("Specified %ls value is not supported.\n"
	"Available values are:\n", name);
    for (size_t i = base; i < menu.size(); ++i)
	LOG("%d: %ls\n", i, CF2W(menu.at(i)).c_str());
    throw std::runtime_error("");
}

namespace Param {
    const wchar_t * const kMethod = L"Target Format";
    const wchar_t * const kBitRate = L"Bit Rate";
    const wchar_t * const kQuality = L"Quality";
    const wchar_t * const kSampleRate = L"Sample Rate";
}

static
int get_bitrate_index(const CFArrayT<CFStringRef> &menu, int rate)
{
    for (int i = menu.size() - 1; i >= 0; --i) {
	std::wstring s = CF2W(menu.at(i));
	int low, high;
	int rc = std::swscanf(s.c_str(), L"%d - %d", &low, &high);
	if (rc == 1 && rate == low)
	    return i;
	else if (rc == 2 && rate >= low && rate <= high)
	    return i;
    }
    return -1;
}

static
int get_highest_bitrate(const CFArrayT<CFStringRef> &menu)
{
    for (int i = menu.size() - 1; i >= 0; --i) {
	std::wstring s = CF2W(menu.at(i));
	int v;
	if (std::swscanf(s.c_str(), L"%d", &v) == 1) return v;
    }
    return 0;
}

template <typename T>
struct CloserTo {
    const T &value_;
    CloserTo(const T &value): value_(value) {}
    bool operator()(const T& lhs, const T& rhs)
    {
	return std::abs(lhs - value_) < std::abs(rhs - value_);
    }
};

static
void setup_sample_rate(AACEncoder &encoder, const Options &opts,
	const std::vector<int> &sample_rate_table)
{
    int rate = opts.rate;
    if (rate != 0) {
	int srcrate = static_cast<int>(
		encoder.getInputBasicDescription().mSampleRate);
	if (rate < 0)
	    rate = srcrate;
	std::vector<int>::const_iterator it
	    = std::min_element(sample_rate_table.begin(),
		    sample_rate_table.end(), CloserTo<int>(rate));
	rate = *it;
    }
    AudioStreamBasicDescription oasbd = encoder.getOutputBasicDescription();
    oasbd.mSampleRate = rate;
    encoder.setOutputBasicDescription(oasbd);
}

static
void set_codec_options(AACEncoder &encoder, const Options &opts)
{
    CFArrayT<CFStringRef> menu, limits;

    // build sampling rate table
    {
	encoder.getParameterRange(Param::kSampleRate, &menu, &limits);
	double v;
	std::vector<int> sample_rate_table;
	for (size_t i = 0; i < limits.size(); ++i)
	    if (std::swscanf(CF2W(limits.at(i)).c_str(), L"%lf", &v) == 1)
		sample_rate_table.push_back(static_cast<int>(v * 1000));
	setup_sample_rate(encoder, opts, sample_rate_table);
    }
    // encoding method
    {
	unsigned method = opts.method;
	if (opts.isSBR()) {
	    int mapping[] = { 1, 0xff, 2, 0 };
	    method = mapping[opts.method];
	}
	encoder.getParameterRange(Param::kMethod, &menu);
	if (method >= menu.size())
	    parameter_not_supported(Param::kMethod, menu);
	else
	    encoder.setEncoderParameter(Param::kMethod, method);
    }
    // bitrate
    {
	encoder.getParameterRange(Param::kBitRate, &menu, &limits);
	if (opts.method == Options::kTVBR) {
	    if (opts.bitrate > 127)
		throw std::runtime_error("TVBR parameter too large");
	    else
		encoder.setEncoderParameter(Param::kBitRate, opts.bitrate);
	} else  {
	    int bitrate = opts.bitrate;
	    if (bitrate == 0) bitrate = get_highest_bitrate(limits);
	    if (get_bitrate_index(limits, bitrate) < 0)
		parameter_not_supported(Param::kBitRate, limits, 1);
	    else {
		size_t index = get_bitrate_index(menu, bitrate);
		encoder.setEncoderParameter(Param::kBitRate, index);
	    }
	}
    }
    // quality
    {
	encoder.getParameterRange(Param::kQuality, &menu);
	if (opts.quality >= menu.size())
	    parameter_not_supported(Param::kQuality, menu);
	else
	    encoder.setEncoderParameter(Param::kQuality, opts.quality);
    }
#if 0
    {
	extern void dump_object(CFTypeRef ref, std::ostream &os);
	CFArrayT<CFDictionaryRef> settings;
	encoder.getCodecSpecificSettingsArray(&settings);
	dump_object(settings, std::cerr);
    }
#endif
}

static
const char *get_strategy_name(int index, uint32_t codec)
{
    const char *lc_table[] = { "ABR", "TVBR", "CVBR", "CBR" };
    const char *he_table[] = { "CBR", "ABR", "CVBR" };
    return codec == 'aach' ? he_table[index] : lc_table[index];
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
std::wstring get_encoder_config(AACEncoder &encoder)
{
    std::wstring s;
    uint32_t codec = encoder.getOutputBasicDescription().mFormatID;
    s = m2w(get_codec_version(codec));
    if (codec != 'aac ' && codec != 'aach')
	return s;

    CFArrayT<CFStringRef> menu;
    int value = encoder.getParameterRange(Param::kMethod, &menu);
    s += format(L", %s", CF2W(menu.at(value)).c_str());

    value = encoder.getParameterRange(Param::kBitRate, &menu);
    if (menu.size())
	s += format(L" %skbps", CF2W(menu.at(value)).c_str());
    else
	s += format(L" q%d", value);

    value = encoder.getParameterRange(Param::kQuality, &menu);
    s += format(L", %ls", CF2W(menu.at(value)).c_str());
    return s;
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
    const wchar_t *ext = opts.isMP4() ? L"m4a" : L"aac";
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
    return h ? format("%02d:%02d:%02d.%03d", h, m, s, millis)
	     : format("%02d:%02d.%03d", m, s, millis);
}

class Timer {
    DWORD m_ticks;
public:
    Timer() { m_ticks = GetTickCount(); };
    double ellapsed() {
	return (static_cast<double>(GetTickCount()) - m_ticks) / 1000.0;
    }
};

static
void do_encode(AACEncoder &encoder, const std::wstring &ofilename,
	const Options &opts)
{
#ifdef _DEBUG
    AudioChannelLayoutX layout;
    encoder.getInputChannelLayout(&layout);
    LOG("Input Channel Layout: %08x\n", layout->mChannelLayoutTag);

    encoder.getChannelLayout(&layout);
    LOG("Output Channel Layout: %08x\n", layout->mChannelLayoutTag);
#endif
    ISink *sink;

    std::wstring ofilenamex = ofilename;
    if (!opts.no_optimize) ofilenamex = L"qaac.int";

    if (opts.is_adts)
	sink = new ADTSSink(ofilename, encoder);
    else if (opts.isALAC())
	sink = new ALACSink(ofilenamex, encoder, !opts.no_optimize);
    else if (opts.isAAC())
	sink = new MP4Sink(ofilenamex, encoder, !opts.no_optimize);
    x::shared_ptr<ISink> sinkp(sink);
    encoder.setSink(sinkp);

    typedef x::shared_ptr<std::FILE> file_t;
    file_t statfp;
    if (opts.save_stat) {
	std::wstring statname = PathReplaceExtension(ofilename, L".stat.txt");
	statfp = file_t(wfopenx(statname.c_str(), L"w"), std::fclose);
    }
    PeriodicDisplay disp(100, opts.verbose);
    uint32_t rate = encoder.getInputBasicDescription().mSampleRate;
    uint64_t tsamples = encoder.src()->length();
    double tseconds = static_cast<double>(tsamples) / rate;
    std::string tstamp = tsamples == -1 ? "-" : formatSeconds(tseconds);

    try {
	Timer timer;
	while (encoder.encodeChunk(1)) {
	    double processed = encoder.samplesRead();
	    double percent = 100.0 * processed / tsamples;
	    double pseconds = processed / rate;
	    double ellapsed = timer.ellapsed();
	    double eta = ellapsed * (tsamples/processed - 1);

	    disp.put(format("\r%s%s/%s (%.1fx), ETA %s   ",
		tsamples == -1 ? "" : format("[%.1f%%] ", percent).c_str(),
		formatSeconds(pseconds).c_str(),
		tstamp.c_str(),
		pseconds / ellapsed,
		tsamples == -1 ? "-" : formatSeconds(eta).c_str()));
	    if (statfp.get())
		std::fprintf(statfp.get(), "%g\n", encoder.currentBitrate());
	}
	uint64_t processed = encoder.samplesRead();
	double pseconds = static_cast<double>(processed) / rate;
	double ellapsed = timer.ellapsed();
	disp.flush();
	if (opts.verbose) fputc('\n', stderr);
	LOG("%" PRId64 "/%" PRId64 " samples processed\n", processed, tsamples);
	LOG("Encoding finished in %s (%.1fx)\n",
		formatSeconds(ellapsed).c_str(), pseconds / ellapsed);
    } catch (const std::exception &e) {
	LOG("\n%s\n", e.what());
    }
}

static
x::shared_ptr<ISource> open_source(const Options &opts)
{
    StdioChannel channel(opts.ifilename);
    InputStream stream(channel);

    if (opts.ignore_length)
	return x::shared_ptr<ISource>(new WaveSource(stream, true));

    if (opts.is_raw) {
	SampleFormat sf(nallow(opts.raw_format).c_str(),
		opts.raw_channels, opts.raw_sample_rate);
	return x::shared_ptr<ISource>(new RawSource(stream, sf));
    }

    if (!stream.seekable() && opts.libsndfile.loaded()) {
	return x::shared_ptr<ISource>(
		new LibSndfileSource(opts.libsndfile, opts.ifilename));
    } else {
	try {
	    return x::shared_ptr<ISource>(new WaveSource(stream, false));
	} catch (const std::runtime_error&) {
	    stream.rewind();
	}
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
    if (opts.libsndfile.loaded()) {
	try {
	    return x::shared_ptr<ISource>(
		    new LibSndfileSource(opts.libsndfile, opts.ifilename));
	} catch (const std::runtime_error&) {
	    stream.rewind();
	}
    }
    try {
	return x::shared_ptr<ISource>(
		new QTMovieSource(opts.ifilename, true));
    } catch (const std::runtime_error&) {
	stream.rewind();
	throw;
    }
}

static
void write_tags(MP4SinkBase *sink,
	const Options &opts, AACEncoder &encoder,
	const std::wstring &encoder_config)
{
    TagEditor editor;

    ITagParser *tp = dynamic_cast<ITagParser*>(encoder.src());
    if (tp) {
	editor.setTag(tp->getTags());
	const std::vector<std::pair<std::wstring, int64_t> > *chapters
	    = tp->getChapters();
	if (chapters)
	    editor.setChapters(*chapters);
    }
    if (!opts.is_raw && std::wcscmp(opts.ifilename, L"-")) {
	try {
	    editor.fetchAiffID3Tags(opts.ifilename);
	} catch (const std::exception &) {}
    }
    editor.setTag(opts.tagopts);
    editor.setTag(Tag::kTool,
	opts.encoder_name + L", " + encoder_config);
    if (opts.isAAC()) {
	GaplessInfo info;
	encoder.getGaplessInfo(&info);
	editor.setGaplessInfo(info);
    }
    for (size_t i = 0; i < opts.artworks.size(); ++i)
	editor.addArtwork(opts.artworks[i].c_str());
    sink->saveTags(editor);
}

static void do_optimize(MP4FileX *file,
	const std::wstring &dst, const Options &opts)
{
    try {
	file->FinishWriteX();
	MP4FileCopy optimizer(file);
	optimizer.start(w2m(dst, utf8_codecvt_facet()).c_str());
	uint64_t total = optimizer.getTotalChunks();
	PeriodicDisplay disp(100, opts.verbose);
	for (uint64_t i = 1; optimizer.copyNextChunk(); ++i) {
	    disp.put(format("\r%" PRId64 "/%" PRId64
		    " chunks written (optimizing)", i, total).c_str());
	}
	disp.flush();
	if (opts.verbose) std::putc('\n', stderr);
    } catch (mp4v2::impl::Exception *e) {
	handle_mp4error(e);
    }
}

static
x::shared_ptr<ISource> do_resample(
    const x::shared_ptr<ISource> &src, const Options &opts,
    uint32_t rate)
{
    LOG("Resampling with libspeexdsp, quality %d\n", opts.src_mode);
    if (!opts.libspeexdsp.get_input_latency) {
	LOG("WARNING: lacking speex_resampler_get_input_latency().\n"
	    "Using guess... might not be quite acculate\n");
    }
    SpeexResampler *resampler = new SpeexResampler(opts.libspeexdsp, src,
	    rate, opts.src_mode);
    x::shared_ptr<ISource> new_src(resampler);
    uint64_t n = 0, rc;
    PeriodicDisplay disp(100, opts.verbose);
    uint32_t srate = src->getSampleFormat().m_rate;
    uint64_t tsamples = src->length();
    double tseconds = static_cast<double>(tsamples) / srate;
    std::string tstamp = tsamples == -1 ? "-" : formatSeconds(tseconds);
    Timer timer;

    while ((rc = resampler->convertSamples(4096)) > 0) {
	n += rc;
	double processed = resampler->samplesRead();
	double percent = 100.0 * processed / tsamples;
	double pseconds = processed / srate;
	double ellapsed = timer.ellapsed();
	double eta = ellapsed * (tsamples/processed - 1);

	disp.put(format("\r%s%s/%s (%.1fx), ETA %s [resampling]  ",
	    tsamples == -1 ? "" : format("[%.1f%%] ", percent).c_str(),
	    formatSeconds(pseconds).c_str(),
	    tstamp.c_str(),
	    pseconds / ellapsed,
	    tsamples == -1 ? "-" : formatSeconds(eta).c_str()));
    }
    disp.flush();
    if (opts.verbose) fputc('\n', stderr);
    LOG("Done rate conversion.\n");
    if (resampler->getPeak() > 1.0) {
	LOG("Peak value %g > 1.0, gain compressed.\n",
	    resampler->getPeak());
    }
    return new_src;
}

static
void encode_file(const x::shared_ptr<ISource> &src,
	const std::wstring &ofilename, const Options &opts,
	bool resampled=false)
{
    x::shared_ptr<ISource> srcx(src);
    if (opts.chanmap.size()) {
	if (opts.chanmap.size() != src->getSampleFormat().m_nchannels)
	    throw std::runtime_error(
		    "nchannels of input and --chanmap spec unmatch");
	srcx = x::shared_ptr<ISource>(new ChannelMapper(src, opts.chanmap));
    }
    AACEncoder encoder(srcx, opts.output_format);
    encoder.setRenderQuality(kQTAudioRenderQuality_Max);

    if (opts.isAAC() && !opts.native_chanmapper && opts.downmix <= 0)
	encoder.forceAACChannelMapping();

    AudioStreamBasicDescription iasbd, oasbd;
    iasbd = encoder.getInputBasicDescription();
    oasbd = encoder.getOutputBasicDescription();

    if (opts.downmix > 0)
	oasbd.mChannelsPerFrame = opts.downmix;
    if (!opts.isAAC() && opts.rate > 0)
	oasbd.mSampleRate = opts.rate;
    encoder.setOutputBasicDescription(oasbd);

    if (opts.isAAC()) {
	set_codec_options(encoder, opts);
	oasbd = encoder.getOutputBasicDescription();
    } else {
	if (oasbd.mChannelsPerFrame != 2)
	    throw std::runtime_error("Only 2ch encoding is supported for ALAC");
	if (iasbd.mBitsPerChannel != 16 && iasbd.mBitsPerChannel != 24)
	    LOG("WARNING: Only 16/24bit format is supported for ALAC");
    }
    std::wstring encoder_config = get_encoder_config(encoder);

    if (!resampled) {
	LOG("%ls\n", encoder_config.c_str());
	if (opts.rate > 0) {
	    if (opts.rate != oasbd.mSampleRate)
		LOG("WARNING: Sample rate will be %gHz\n",
			oasbd.mSampleRate);
	} else if (iasbd.mSampleRate != oasbd.mSampleRate)
	    LOG("WARNING: Resampled to %gHz\n", oasbd.mSampleRate);
	if (opts.isAAC()) {
	    if (iasbd.mChannelsPerFrame == 3)
		LOG("WARNING: Downmixed to 2ch\n");
	}
    }
    if (iasbd.mSampleRate != oasbd.mSampleRate &&
	    opts.libspeexdsp.loaded() && !opts.native_resampler) {
	x::shared_ptr<ISource> srcx
	    = do_resample(src, opts, oasbd.mSampleRate);
	encode_file(srcx, ofilename, opts, true);
	return;
    }
    do_encode(encoder, ofilename, opts);
    if (encoder.framesWritten())
	LOG("Overall bitrate: %gkbps\n", encoder.overallBitrate());

    MP4SinkBase *sink = dynamic_cast<MP4SinkBase*>(encoder.sink());
    if (sink) {
	write_tags(sink, opts, encoder, encoder_config);
	if (!opts.no_optimize)
	    do_optimize(sink->getFile(), ofilename, opts);
    }
}

std::wstring load_cue_sheet(const wchar_t *name)
{
    StdioChannel channel(name);
    InputStream stream(channel);
    int64_t size = stream.size();
    if (size > 0x100000)
	throw std::runtime_error("Cuesheet is too big");
    std::vector<char> buffer(size + 2);
    stream.read(&buffer[0], size);
    buffer[size] = buffer[size+1] = 0;

    if (!std::memcmp(&buffer[0], "\xef\xbb\xbf", 3))
	return m2w(&buffer[3], utf8_codecvt_facet());
    else if (!std::memcmp(&buffer[0], "\xff\xfe", 2)) {
	return std::wstring(reinterpret_cast<wchar_t*>(&buffer[2]));
    }
    std::wstring result;
    try {
	result = m2w(&buffer[0], utf8_codecvt_facet());
    } catch (...) {
	result = m2w(&buffer[0]);
    }
    return result;
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

    std::wstringbuf istream(load_cue_sheet(opts.ifilename));
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
	encode_file(source, ofilename, opts);
    }
}

static
void install_aach_codec()
{
    HKEY hKey;
    LPCWSTR key = L"SOFTWARE\\Apple Computer, Inc.\\QuickTime";
    HRESULT hr = RegOpenKeyExW(HKEY_LOCAL_MACHINE, key, 0, KEY_READ, &hKey);
    if (hr != ERROR_SUCCESS)
	throw_win32_error(format("RegOpenKeyExW: %ls", key), hr);
    x::shared_ptr<HKEY__> hKey__(hKey, RegCloseKey);

    DWORD size;
    key = L"QTSysDir";
    RegQueryValueExW(hKey, key, 0, 0, 0, &size);
    std::vector<BYTE> buffer(size);
    RegQueryValueExW(hKey, key, 0, 0, &buffer[0], &size);
    std::wstring path = format(L"%s\\QuickTimeAudioSupport.qtx", &buffer[0]);

    HMODULE hModule = LoadLibraryW(path.c_str());
    if (!hModule)
	throw_win32_error(format("LoadLibraryW: %ls", path.c_str()),
	       GetLastError());	
    ComponentRoutineProcPtr proc
	= ProcAddress(hModule, "ACMP4AACHighEfficiencyEncoderEntry"); 
    ComponentDescription desc = { 'aenc', 'aach', 'appl', 0 };
    RegisterComponent(&desc, proc, 0, 0, 0, 0);
}

const char *get_qaac_version();

std::wstring get_module_directory()
{
    std::wstring selfpath = GetModuleFileNameX();
    const wchar_t *fpos = PathFindFileNameW(selfpath.c_str());
    return selfpath.substr(0, fpos - selfpath.c_str());
}

static
void load_modules(Options &opts)
{
    opts.libsndfile = LibSndfileModule(L"libsndfile-1.dll");
    opts.libflac = FLACModule(L"libFLAC.dll");
    opts.libwavpack = WavpackModule(L"wavpackdll.dll");
    opts.libspeexdsp = SpeexResamplerModule(L"libspeexdsp_vc10.dll");
    if (!opts.libspeexdsp.loaded())
	opts.libspeexdsp = SpeexResamplerModule(L"libspeexdsp.dll");
}

extern void OverrideRegistryWith(const x::shared_ptr<FILE> &fp);

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
    try {
	if (!opts.parse(argc, argv))
	    return 1;

	if (opts.verbose)
	    Log::instance()->enable_stderr();
	if (opts.logfilename)
	    Log::instance()->enable_file(opts.logfilename);

	if (opts.nice)
	    SetPriorityClass(GetCurrentProcess(), IDLE_PRIORITY_CLASS);

	override_registry();
	LOG("initializing QTML...");
	QTInitializer __quicktime__;
	LOG("done\n\n");

	uint32_t qtver;
	Gestalt(gestaltQuickTime, reinterpret_cast<long*>(&qtver));
	qtver >>= 16;
	std::string encoder_name = format("qaac %s, QuickTime %d.%d.%d",
		get_qaac_version(),
		qtver >> 8, (qtver >> 4) & 0xf, qtver & 0xf);
	opts.encoder_name = widen(encoder_name);
	LOG("%s\n", encoder_name.c_str());

	if (opts.isSBR())
	    install_aach_codec();
	load_modules(opts);

	mp4v2::impl::log.setVerbosity(MP4_LOG_NONE);
	//mp4v2::impl::log.setVerbosity(MP4_LOG_VERBOSE4);

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
