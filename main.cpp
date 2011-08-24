#include <iostream>
#include <ctime>
#include <cstdarg>
#include <clocale>
#include <algorithm>
#include <functional>
#include <QTML.h>
#include <aifffile.h>
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

#include <crtdbg.h>

class PeriodicDisplay {
    FILE *m_fp;
    uint32_t m_interval;
    uint32_t m_last_tick;
    std::string m_message;
public:
    PeriodicDisplay(FILE *fp, uint32_t interval)
	: m_fp(fp),
	  m_interval(interval),
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
	std::fputs(m_message.c_str(), m_fp);
	std::fflush(m_fp);
    }
};

static
void parameter_not_supported(const wchar_t *name,
	const CFArrayT<CFStringRef> &menu, size_t base=0)
{
    std::fprintf(stderr, "Specified %ls value is not supported.\n"
			"Available values are:\n", name);
    for (size_t i = base; i < menu.size(); ++i)
	std::fprintf(stderr, "%d: %ls\n", i, CF2W(menu.at(i)).c_str());
    std::exit(2);
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
void setup_sample_rate(AACEncoder &encoder, const Options &opts)
{
    int rate = opts.rate;
    if (rate != 0) {
	int srcrate = static_cast<int>(
		encoder.getInputBasicDescription().mSampleRate);
	if (rate < 0)
	    rate = srcrate;
	std::vector<int>::const_iterator it
	    = std::min_element(opts.sample_rate_table.begin(),
		    opts.sample_rate_table.end(), CloserTo<int>(rate));
	rate = *it;
    }
    AudioStreamBasicDescription oasbd = encoder.getOutputBasicDescription();
    oasbd.mSampleRate = rate;
    encoder.setOutputBasicDescription(oasbd);
}

static
void set_codec_options(AACEncoder &encoder, Options &opts)
{
    CFArrayT<CFStringRef> menu, limits;

    if (opts.downmix > 0)
    {
	AudioStreamBasicDescription oasbd = encoder.getOutputBasicDescription();
	oasbd.mChannelsPerFrame = opts.downmix;
	encoder.setOutputBasicDescription(oasbd);
    }
    // build sampling rate table
    {
	encoder.getParameterRange(Param::kSampleRate, &menu, &limits);
	double v;
	for (size_t i = 0; i < limits.size(); ++i)
	    if (std::swscanf(CF2W(limits.at(i)).c_str(), L"%lf", &v) == 1)
		opts.sample_rate_table.push_back(static_cast<int>(v * 1000));
	setup_sample_rate(encoder, opts);
    }
    // encoding method
    {
	unsigned method = opts.method;
	if (opts.isSBR()) {
	    int mapping[] = { 1, 0xff, 2, 0 };
	    method = mapping[opts.method];
	    opts.encoder_name += L", HE";
	}
	encoder.getParameterRange(Param::kMethod, &menu);
	if (method >= menu.size())
	    parameter_not_supported(Param::kMethod, menu);
	else {
	    encoder.setEncoderParameter(Param::kMethod, method);
	    std::wstring s = CF2W(menu.at(method));
	    opts.used_settings.push_back(format("Method: %ls", s.c_str()));
	    static const char *method_name[] = {
		"ABR", "TVBR", "CVBR", "CBR"
	    };
	    opts.encoder_name
		+= widen(format(", %s", method_name[opts.method]));
	}
    }
    // bitrate
    {
	size_t n = encoder.getParameterRange(Param::kBitRate,
		&menu, &limits);
	if (opts.method == Options::kTVBR) {
	    if (opts.bitrate > n)
		throw std::runtime_error("TVBR parameter too large");
	    else {
		encoder.setEncoderParameter(Param::kBitRate, opts.bitrate);
		opts.used_settings.push_back(
		    format("TVBR Quality: %d", opts.bitrate));
		opts.encoder_name += format(L" Quality %d", opts.bitrate);
	    }
	} else  {
	    int index = get_bitrate_index(limits, opts.bitrate);
	    if (index < 0)
		parameter_not_supported(Param::kBitRate, limits, 1);
	    else {
		index = get_bitrate_index(menu, opts.bitrate);
		encoder.setEncoderParameter(Param::kBitRate, index);
		std::wstring s = CF2W(menu.at(index));
		opts.used_settings.push_back(
			format("Bitrate: %ls", s.c_str()));
		opts.encoder_name += format(L" Bitrate %s", s.c_str());
	    }
	}
    }
    // quality
    {
	encoder.getParameterRange(Param::kQuality, &menu);
	if (opts.quality >= menu.size())
	    parameter_not_supported(Param::kQuality, menu);
	else {
	    encoder.setEncoderParameter(Param::kQuality, opts.quality);
	    opts.used_settings.push_back(
		format("Quality: %ls",
		    CF2W(menu.at(opts.quality)).c_str()));
	}
    }
#if 0
    {
	extern void dump_object(CFTypeRef ref, std::ostream &os);
	CFArrayT<CFDictionaryRef> settings;
	encoder.getCodecSpecificSettingsArray(&settings);
	dump_object(settings, std::cout);
    }
#endif
}

static
std::wstring get_output_filename(const wchar_t *ifilename, Options &opts)
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
    return format("%02d:%02d:%02d.%03d", h, m, s, millis);
}

static
void do_encode(AACEncoder &encoder, const std::wstring &ofilename,
	const Options &opts)
{
#ifdef _DEBUG
    AudioChannelLayoutX layout;
    encoder.getInputChannelLayout(&layout);
    fprintf(stderr, "Input Channel Layout: %08x\n", layout->mChannelLayoutTag);

    encoder.getChannelLayout(&layout);
    fprintf(stderr, "Output Channel Layout: %08x\n", layout->mChannelLayoutTag);
#endif
    ISink *sink;

    std::wstring ofilenamex(ofilename);
    if (!opts.no_optimize && opts.isMP4())
	ofilenamex += L".tmp";
    if (opts.is_adts)
	sink = new ADTSSink(ofilenamex, encoder);
    else if (opts.isALAC())
	sink = new ALACSink(ofilenamex, encoder);
    else if (opts.isAAC())
	sink = new MP4Sink(ofilenamex, encoder);
    std::auto_ptr<ISink> __delete_later__(sink);
    encoder.setSink(*sink);

    typedef boost::shared_ptr<std::FILE> file_t;
    file_t statfp;
    if (opts.save_stat) {
	std::wstring statname = PathReplaceExtension(ofilename, L".stat.txt");
	statfp = file_t(wfopenx(statname.c_str(), L"w"), std::fclose);
    }
    PeriodicDisplay disp(stderr, 100);
    try {
	uint32_t rate = encoder.getInputBasicDescription().mSampleRate;
	uint64_t total_samples = encoder.src()->length();
	double total_seconds = static_cast<double>(total_samples) / rate;

	while (encoder.encodeChunk(1)) {
	    if (opts.verbose && !opts.logfilename) {
		uint64_t samplesRead = encoder.samplesRead();
		double read_seconds = static_cast<double>(samplesRead) / rate;

		disp.put(format("\r%s / %s processed",
		    formatSeconds(read_seconds).c_str(),
		    total_samples == -1
		    ? "-"
		    : formatSeconds(total_seconds).c_str()));
	    }
	    if (statfp.get())
		std::fprintf(statfp.get(), "%g\n", encoder.currentBitrate());
	}
	if (opts.verbose && !opts.logfilename) disp.flush();
    } catch (const std::exception &e) {
	std::fprintf(stderr, "\n%s\n", e.what());
    }
}

static
ISource *open_source(const Options &opts)
{
    StdioChannel channel(opts.ifilename);
    InputStream stream(channel);

    if (opts.ignore_length)
	return new WaveSource(stream, true);

    if (opts.is_raw) {
	SampleFormat sf(nallow(opts.raw_format).c_str(),
		opts.raw_channels, opts.raw_sample_rate);
	return new RawSource(stream, sf);
    }

    if (!stream.seekable() && opts.libsndfile.loaded()) {
	return new LibSndfileSource(opts.libsndfile, opts.ifilename);
    } else {
	try {
	    return new WaveSource(stream, false);
	} catch (const std::runtime_error&) {
	    stream.rewind();
	}
    }

    if (opts.libflac.loaded()) {
	try {
	    return new FLACSource(opts.libflac, stream);
	} catch (const std::runtime_error&) {
	    stream.rewind();
	}
    }
    if (opts.libwavpack.loaded()) {
	try {
	    return new WavpackSource(opts.libwavpack, stream);
	} catch (const std::runtime_error&) {
	    stream.rewind();
	}
    }
    if (opts.libsndfile.loaded()) {
	try {
	    return new LibSndfileSource(opts.libsndfile, opts.ifilename);
	} catch (const std::runtime_error&) {
	    stream.rewind();
	}
    }
    try {
	return new QTMovieSource(opts.ifilename, true);
    } catch (const std::runtime_error&) {
	stream.rewind();
	throw;
    }
}

static void fetch_aiff_id3_tags(const Options &opts, TagEditor &editor)
{
    try {
	std::wstring fullname = get_prefixed_fullpath(opts.ifilename);
	TagLib::RIFF::AIFF::File file(fullname.c_str());
	if (!file.isOpen())
	    throw std::runtime_error("taglib: can't open file");
	TagLib::ID3v2::Tag *tag = file.tag();
	const TagLib::ID3v2::FrameList &frameList = tag->frameList();
	TagLib::ID3v2::FrameList::ConstIterator it;
	for (it = frameList.begin(); it != frameList.end(); ++it) {
	    TagLib::ByteVector vID = (*it)->frameID();
	    std::string sID(vID.data(), vID.data() + vID.size());
	    std::wstring value = (*it)->toString().toWString();
	    uint32_t id = ID3::GetIDFromTagName(sID.c_str());
	    if (id) {
		if (id == Tag::kGenre) {
		    wchar_t *endp;
		    long n = std::wcstol(value.c_str(), &endp, 10);
		    if (!*endp) {
			id = Tag::kGenreID3;
			value = widen(format("%d", n + 1));
		    }
		}
		editor.setTag(id, value);
	    }
	}
    } catch (...) {}
}

static
void write_tags(const std::wstring &ofilename,
	const Options &opts, AACEncoder &encoder)
{
    std::wstring ofilenamex(ofilename);
    if (!opts.no_optimize)
	ofilenamex += L".tmp";

    TagEditor editor(ofilenamex);

    ITagParser *tp = dynamic_cast<ITagParser*>(encoder.src());
    if (tp) {
	editor.setTag(tp->getTags());
	const std::vector<std::pair<std::wstring, int64_t> > *chapters
	    = tp->getChapters();
	if (chapters)
	    editor.setChapters(*chapters);
    }
    if (!opts.is_raw && std::wcscmp(opts.ifilename, L"-"))
	fetch_aiff_id3_tags(opts, editor);
    editor.setTag(opts.tagopts);
    editor.setTag(Tag::kTool, opts.encoder_name);
    if (opts.isAAC()) {
	GaplessInfo info;
	encoder.getGaplessInfo(&info);
	editor.setGaplessInfo(info);
    }
    for (size_t i = 0; i < opts.artworks.size(); ++i)
	editor.addArtwork(opts.artworks[i].c_str());
    editor.save();
    if (!opts.no_optimize) {
	mp4v2::impl::MP4File optimizer;
	std::string utf8_name = w2m(ofilename, utf8_codecvt_facet());
	std::string tmpname = utf8_name + ".tmp";
	try {
	    optimizer.Optimize(tmpname.c_str(), utf8_name.c_str());
	    DeleteFileX(ofilenamex.c_str());
	} catch (mp4v2::impl::Exception *e) {
	    handle_mp4error(e);
	}
    }
}

static
void encode_file(ISource *src, const std::wstring &ofilename, Options &opts,
	bool resample=false)
{
    opts.reset();
    AACEncoder encoder(src, opts.output_format);

    if (opts.isAAC())
        set_codec_options(encoder, opts);

    AudioStreamBasicDescription iasbd, oasbd;
    iasbd = encoder.getInputBasicDescription();
    oasbd = encoder.getOutputBasicDescription();

    if (!opts.isAAC() && opts.rate > 0) {
	oasbd.mSampleRate = opts.rate;
	encoder.setOutputBasicDescription(oasbd);
	oasbd = encoder.getOutputBasicDescription();
    }
    if (opts.verbose && !resample) {
	if (opts.rate > 0) {
	    if (opts.rate != oasbd.mSampleRate)
		std::fprintf(stderr, "WARNING: Sample rate will be %gHz\n",
			oasbd.mSampleRate);
	} else if (iasbd.mSampleRate != oasbd.mSampleRate)
	    std::fprintf(stderr, "WARNING: Resampled to %gHz\n",
		    oasbd.mSampleRate);
	if (opts.isAAC()) {
	    if (iasbd.mChannelsPerFrame == 3)
		std::fprintf(stderr, "WARNING: Downmixed to 2ch\n");
	    if (opts.is_first_file)
		for (size_t i = 0; i < opts.used_settings.size(); ++i)
		    std::fprintf(stderr, "%s\n", opts.used_settings[i].c_str());
	}
    }
    if (iasbd.mSampleRate != oasbd.mSampleRate &&
	    opts.libspeexdsp.loaded() && !opts.native_resampler) {
	if (opts.verbose) {
	    std::fprintf(stderr,
		"Resampling with libspeexdsp, quality %d\n", opts.src_mode);
	    if (!opts.libspeexdsp.get_input_latency) {
		std::fprintf(stderr,
		    "WARNING: lacking speex_resampler_get_input_latency().\n"
		    "Using guess... might not be quite acculate\n");
	    }
	}
	SpeexResampler*resampler = new SpeexResampler(opts.libspeexdsp, src,
		oasbd.mSampleRate, opts.src_mode);
	std::auto_ptr<ISource> srcx(resampler);
	uint64_t n = 0, rc;
        PeriodicDisplay disp(stderr, 100);
	while ((rc = resampler->convertSamples(4096)) > 0) {
	    n += rc;
	    if (opts.verbose && !opts.logfilename)
		disp.put(format("\r%" PRId64 " samples processed", n));
	}
	if (opts.verbose) {
	    if (!opts.logfilename) disp.flush();
	    std::fprintf(stderr, "\nDone rate conversion.\n");
	    if (resampler->getPeak() > 1.0) {
		std::fprintf(stderr,
			"Peak value %g > 1.0, gain compressed.\n",
			resampler->getPeak());
	    }
	}
	encode_file(srcx.get(), ofilename, opts, true);
	return;
    }
    do_encode(encoder, ofilename, opts);
    if (encoder.framesWritten()) {
	if (opts.verbose)
	    std::fprintf(stderr, "\nOverall bitrate: %gkbps\n",
		    encoder.overallBitrate());
	if (opts.isMP4())
	    write_tags(ofilename, opts, encoder);
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
	opts.reset();
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
	opts.tagopts = track_tags;

	CompositeSource source;
	std::wstring ifilename;
	ISource *src;
	for (size_t j = 0; j < track.m_segments.size(); ++j) {
	    CueSegment &seg = track.m_segments[j];
	    if (seg.m_filename == L"__GAP__") {
		opts.ifilename = L"GAP";
		if (!src)
		    throw std::runtime_error("Pre/Postgap command before song");
		src = new NullSource(src->getSampleFormat());
	    } else {
		ifilename = PathCombineX(cuedir, seg.m_filename);
		opts.ifilename = const_cast<wchar_t*>(ifilename.c_str());
		src = open_source(opts);
	    }
	    int64_t begin = static_cast<int64_t>(seg.m_begin) * 588;
	    int64_t end = -1;
	    if (seg.m_end != -1)
		end = static_cast<int64_t>(seg.m_end) * 588 - begin;
	    IPartialSource *psrc = dynamic_cast<IPartialSource*>(src);
	    if (!psrc) {
		delete src;
		throw std::runtime_error(
		    "Can't process partial input with this file type");
	    }
	    psrc->setRange(begin, end);
	    source.addSource(src);
	}
	std::wstring formatstr = opts.fname_format
	    ? opts.fname_format : L"${tracknumber}${title& }${title}";
	std::wstring ofilename =
	    process_template(formatstr, TagLookup(track, track_tags));
	ofilename = strtransform(ofilename, FNConverter()) + L".stub";
	ofilename = get_output_filename(ofilename.c_str(), opts);
	if (opts.verbose)
	    std::fprintf(stderr, "\n%ls\n",
		    PathFindFileNameW(ofilename.c_str()));
	encode_file(&source, ofilename, opts);
	opts.is_first_file = false;
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
    DWORD size;
    key = L"QTSysDir";
    RegQueryValueExW(hKey, key, 0, 0, 0, &size);
    std::vector<wchar_t> buffer(size);
    RegQueryValueExW(hKey, key, 0, 0,
	    reinterpret_cast<LPBYTE>(&buffer[0]), &size);
    RegCloseKey(hKey);
    std::wstring path =
	format(L"%s%s", &buffer[0], L"QuickTimeAudioSupport.qtx");

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

static
void load_modules(Options &opts)
{
    std::wstring selfdir;
#ifndef NOSTRICT_LOADING
    std::wstring selfpath = GetModuleFileNameX();
    const wchar_t *fpos = PathFindFileNameW(selfpath.c_str());
    selfdir = selfpath.substr(0, fpos - selfpath.c_str());
#endif
    opts.libsndfile = LibSndfileModule(selfdir + L"libsndfile-1.dll");
    opts.libflac = FLACModule(selfdir + L"libFLAC.dll");
    opts.libwavpack = WavpackModule(selfdir + L"wavpackdll.dll");
    opts.libspeexdsp = SpeexResamplerModule(selfdir + L"libspeexdsp_vc10.dll");
    if (!opts.libspeexdsp.loaded())
	opts.libspeexdsp = SpeexResamplerModule(selfdir + L"libspeexdsp.dll");
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

    std::setlocale(LC_CTYPE, "");
    std::setbuf(stderr, 0);
#ifdef DEBUG_ATTACH
    FILE *fp = std::fopen("CON", "r");
    std::getc(fp);
#endif
    try {
	if (!opts.parse(argc, argv))
	    return 1;

	if (opts.logfilename)
	    _wfreopen(opts.logfilename, L"w", stderr);

	if (opts.nice)
	    SetPriorityClass(GetCurrentProcess(), IDLE_PRIORITY_CLASS);
	QTInitializer __quicktime__(opts.verbose);

	uint32_t qtver;
	Gestalt(gestaltQuickTime, reinterpret_cast<long*>(&qtver));
	qtver >>= 16;
	std::string encoder_name = format("qaac %s, QuickTime %d.%d.%d",
		get_qaac_version(),
		qtver >> 8, (qtver >> 4) & 0xf, qtver & 0xf);
	opts.encoder_name = opts.encoder_name_ = widen(encoder_name);
	if (opts.verbose)
	    std::fprintf(stderr, "%s\n", encoder_name.c_str());

	if (opts.isSBR())
	    install_aach_codec();
	load_modules(opts);

	mp4v2::impl::log.setVerbosity(MP4_LOG_NONE);

	while ((opts.ifilename = *argv++)) {
	    opts.reset();

	    if (opts.verbose) {
		const wchar_t *name = L"<stdin>";
		if (std::wcscmp(opts.ifilename, L"-"))
		    name = PathFindFileNameW(opts.ifilename);
		std::fprintf(stderr, "\n%ls\n", name);
	    }
	    size_t len = std::wcslen(opts.ifilename);
	    if (len > 4 && wslower(opts.ifilename + (len - 4)) == L".cue")
		handle_cue_sheet(opts);
	    else {
		std::wstring ofilename
		    = get_output_filename(opts.ifilename, opts);
		std::auto_ptr<ISource> src(open_source(opts));
		encode_file(src.get(), ofilename, opts);
	    }
	    opts.is_first_file = false;
	}
	return 0;

    } catch (const std::exception &e) {
	std::fprintf(stderr, "%s\n", e.what());
	return 2;
    }
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
