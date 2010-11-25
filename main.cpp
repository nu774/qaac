#include <iostream>
#include <cstdarg>
#include <clocale>
#include <algorithm>
#include <functional>
#include <QTML.h>
#include "strcnv.h"
#include "win32util.h"
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
    for (size_t i = 0; i < menu.size(); ++i) {
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
    if (rate == 0)
	return;
    int srcrate = static_cast<int>(
	    encoder.getInputBasicDescription().mSampleRate);
    if (rate < 0)
	rate = srcrate;
    std::vector<int>::const_iterator it
	= std::min_element(opts.sample_rate_table.begin(),
		opts.sample_rate_table.end(), CloserTo<int>(rate));
    AudioStreamBasicDescription oasbd = encoder.getOutputBasicDescription();
    oasbd.mSampleRate = *it;
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
    if (opts.ofilename)
	return opts.ofilename;

    const wchar_t *ext = opts.is_adts ? L"aac" : L"m4a";
    const wchar_t *outdir = opts.outdir ? opts.outdir : L".";
    if (!std::wcscmp(ifilename, L"-"))
	return std::wstring(L"stdin.") + ext;
    else {
	std::wstring obasename =
	    PathReplaceExtension(PathFindFileNameW(ifilename), ext);
	std::wstring ofilename = format(L"%s/%s", outdir, obasename.c_str());
	if (GetFullPathNameX(ifilename) == GetFullPathNameX(ofilename)) {
	    std::wstring tl = 
		opts.isALAC() ? std::wstring(L"alac.") + ext
			      : std::wstring(L"aac.") + ext;
	    ofilename = PathReplaceExtension(ofilename, tl.c_str());
	}
	return ofilename;
    }
}

static
void do_encode(AACEncoder &encoder, const std::wstring &ofilename,
	const Options &opts)
{
    AudioChannelLayoutX layout;
    encoder.getChannelLayout(&layout);
    ISink *sink;

    std::wstring ofilenamex(ofilename);
    if (!opts.no_optimize && !opts.is_adts)
	ofilenamex += L".tmp";
    if (opts.is_adts)
	sink = new ADTSSink(ofilenamex, encoder);
    else if (opts.isALAC())
	sink = new ALACSink(ofilenamex, encoder);
    else if (opts.isAAC())
	sink = new MP4Sink(ofilenamex, encoder);
    std::auto_ptr<ISink> __delete_later__(sink);
    encoder.setSink(*sink);

    typedef std::tr1::shared_ptr<std::FILE> file_t;
    file_t statfp;
    if (opts.isAAC() && opts.save_stat) {
	std::wstring statname = PathReplaceExtension(ofilename, L".stat.txt");
	FILE *fp = _wfopen(statname.c_str(), L"w");
	if (fp) statfp = file_t(fp, std::fclose);
    }
    PeriodicDisplay disp(stderr, 100);
    try {
	while (encoder.encodeChunk(1)) {
	    if (opts.verbose)
		disp.put(format("\r" LL "d/" LL "d samples processed",
		    encoder.samplesRead(), encoder.src()->length()));
	    if (statfp.get())
		std::fprintf(statfp.get(), "%g\n", encoder.currentBitrate());
	}
	if (opts.verbose) disp.flush();
    } catch (const std::exception &e) {
	std::fprintf(stderr, "\n%s\n", e.what());
    }
}

static
ISource *open_source(const Options &opts)
{
    InputStream stream(StdioChannel(opts.ifilename));

    if (opts.ignore_length)
	return new WaveSource(stream, true);

    if (opts.is_raw) {
	SampleFormat sf(nallow(opts.raw_format).c_str(),
		opts.raw_channels, opts.raw_sample_rate);
	return new RawSource(stream, sf);
    }

    if (opts.libsndfile.loaded()) {
	try {
	    return new LibSndfileSource(opts.libsndfile, opts.ifilename);
	} catch (const std::runtime_error&) {
	    if (!stream.seekable()) throw;
	    stream.rewind();
	}
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
    try {
	return new QTMovieSource(opts.ifilename, true);
    } catch (const std::runtime_error&) {
	throw std::runtime_error("Unsupported format");
    }
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

    if (!opts.is_raw && opts.libid3tag.loaded()) {
	try {
	    StdioChannel channel(opts.ifilename);
	    if (channel.seekable()) {
		InputStream stream(channel);
		char magic[12];
		stream.rewind();
		stream.read(magic, 12);
		stream.rewind();

		if (!std::memcmp(magic, "FORM", 4) &&
			!std::memcmp(magic + 8, "AIFF", 4)) {
		    AIFFTagParser parser(opts.libid3tag, stream);
		    editor.setTag(parser.getTags());
		}
	    }
	} catch (...) {}
    }
    editor.setTag(opts.tagopts);
    editor.setTag(Tag::kTool, opts.encoder_name);
    if (opts.isAAC()) {
	GaplessInfo info;
	encoder.getGaplessInfo(&info);
	editor.setGaplessInfo(info);
    }
    editor.save();
    if (!opts.no_optimize) {
	mp4v2::impl::MP4File optimizer(0);
	std::string utf8_name = w2m(ofilename, utf8_codecvt_facet());
	std::string tmpname = utf8_name + ".tmp";
	try {
	    optimizer.Optimize(tmpname.c_str(), utf8_name.c_str());
	    DeleteFileW(ofilenamex.c_str());
	} catch (mp4v2::impl::MP4Error *e) {
	    handle_mp4error(e);
	}
    }
}

static
void encode_file(ISource *src, const std::wstring &ofilename, Options &opts)
{
    AACEncoder encoder(src, opts.output_format);

    if (opts.isAAC()) {
	set_codec_options(encoder, opts);
	if (opts.is_first_file && opts.verbose) {
	    for (size_t i = 0; i < opts.used_settings.size(); ++i)
		std::fprintf(stderr, "%s\n", opts.used_settings[i].c_str());
	}
    }
    if (opts.verbose) {
	AudioStreamBasicDescription iasbd, oasbd;
	iasbd = encoder.getInputBasicDescription();
	oasbd = encoder.getOutputBasicDescription();
	if (opts.rate > 0) {
	    if (opts.rate != oasbd.mSampleRate)
		std::fprintf(stderr, "WARNING: Sample rate will be %gHz\n",
			oasbd.mSampleRate);
	} else if (iasbd.mSampleRate != oasbd.mSampleRate)
	    std::fprintf(stderr, "WARNING: Resampled to %gHz\n",
		    oasbd.mSampleRate);
    }
    do_encode(encoder, ofilename, opts);
    if (encoder.framesWritten()) {
	if (opts.verbose)
	    std::fprintf(stderr, "\nOverall bitrate: %gkbps\n",
		    encoder.overallBitrate());
	if (!opts.is_adts)
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
	wchar_t *p = reinterpret_cast<wchar_t*>(&buffer[2]);
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
	if (std::wcschr(L":/?\\\"", ch))
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
	uint32_t id = GetIDFromTagName(skey.c_str());
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
    ConvertToItunesTags(cue.m_meta, &album_tags, true);
    for (size_t i = 0; i < cue.m_tracks.size(); ++i) {
	opts.reset();
	CueTrack &track = cue.m_tracks[i];
	if (!track.m_segments.size())
	    continue;

	meta_t tmp;
	ConvertToItunesTags(track.m_meta, &tmp);
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
	= reinterpret_cast<ComponentRoutineProcPtr>(
		GetProcAddress(hModule, "ACMP4AACHighEfficiencyEncoderEntry")); 
    ComponentDescription desc = { 'aenc', 'aach', 'appl', 0 };
    RegisterComponent(&desc, proc, 0, 0, 0, 0);
}

const char *get_qaac_version();

static
void load_modules(Options &opts)
{
    std::wstring selfpath = GetModuleFileNameX();
    const wchar_t *fpos = PathFindFileNameW(selfpath.c_str());
    std::wstring selfdir = selfpath.substr(0, fpos - selfpath.c_str());
    opts.libsndfile = LibSndfileModule(selfdir + L"libsndfile_vc71.dll");
    opts.libflac = FLACModule(selfdir + L"libFLAC_vc71.dll");
    opts.libwavpack = WavpackModule(selfdir + L"wavpackdll_vc71.dll");
    opts.libid3tag = LibID3TagModule(selfdir + L"libid3tag_vc71.dll");
}

int wmain(int argc, wchar_t **argv)
{
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

	while (opts.ifilename = *argv++) {
	    opts.reset();

	    std::wstring iname;
	    if (std::wcscmp(opts.ifilename, L"-")) {
		iname = GetFullPathNameX(opts.ifilename);
		opts.ifilename = const_cast<wchar_t*>(iname.c_str());
	    }
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
