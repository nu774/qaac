#include <limits>
#include "options.h"
#include "wgetopt.h"
#include "itunetags.h"

static wide::option long_options[] = {
#ifdef REFALAC
    { L"fast", no_argument, 0, 'afst' },
#else
    { L"formats", no_argument, 0, 'fmts' },
    { L"abr", required_argument, 0, 'a' },
    { L"tvbr", required_argument, 0, 'V' },
    { L"cvbr", required_argument, 0, 'v' },
    { L"cbr", required_argument, 0, 'c' },
    { L"he", no_argument, 0, 'aach' },
    { L"quality", required_argument, 0, 'q' },
    { L"adts", no_argument, 0, 'ADTS' },
    { L"alac", no_argument, 0, 'A' },
    { L"native-resampler", optional_argument, 0, 'nsrc' },
#endif
    { L"check", no_argument, 0, 'chck' },
    { L"decode", no_argument, 0, 'D' },
    { L"no-optimize", no_argument, 0, 'noop' },
    { L"bits-per-sample", required_argument, 0, 'b' },
    { L"rate", required_argument, 0, 'r' },
    { L"lowpass", required_argument, 0, 'lpf ' },
    { L"normalize", no_argument, 0, 'N' },
    { L"gain", required_argument, 0, 'gain' },
    { L"delay", required_argument, 0, 'dlay' },
    { L"matrix-preset", required_argument, 0, 'mixp' },
    { L"matrix-file", required_argument, 0, 'mixm' },
    { L"no-matrix-normalize", no_argument, 0, 'nmxn' },
    { L"chanmap", required_argument, 0, 'cmap' },
    { L"chanmask", required_argument, 0, 'mask' },
    { L"help", no_argument, 0, 'h' },
    { L"silent", no_argument, 0, 's' },
    { L"verbose", no_argument, 0, 'verb' },
    { L"stat", no_argument, 0, 'S' },
    { L"threading", no_argument, 0, 'thrd' },
    { L"nice", no_argument, 0, 'n' },
    { L"tmpdir", required_argument, 0, 'tmpd' },
    { L"text-codepage", required_argument, 0, 'txcp' },
    { L"raw", no_argument, 0, 'R' },
    { L"raw-channels", required_argument, 0,  'Rchn' },
    { L"raw-rate", required_argument, 0,  'Rrat' },
    { L"raw-format", required_argument, 0,  'Rfmt' },
    { L"ignorelength", no_argument, 0, 'i' },
    { L"concat", no_argument, 0, 'cat ' },
    { L"concat-cuesheet", no_argument, 0, 'conc' },
    { L"fname-format", required_argument, 0, 'nfmt' },
    { L"log", required_argument, 0, 'log ' },
    { L"title", required_argument, 0, Tag::kTitle },
    { L"subtitle", required_argument, 0, Tag::kSubTitle },
    { L"artist", required_argument, 0, Tag::kArtist },
    { L"band", required_argument, 0, Tag::kAlbumArtist },
    { L"album", required_argument, 0, Tag::kAlbum },
    { L"grouping", required_argument, 0, Tag::kGrouping },
    { L"composer", required_argument, 0, Tag::kComposer },
    { L"comment", required_argument, 0, Tag::kComment },
    { L"genre", required_argument, 0, Tag::kGenre },
    { L"date", required_argument, 0, Tag::kDate },
    { L"track", required_argument, 0, Tag::kTrack },
    { L"disk", required_argument, 0, Tag::kDisk },
    { L"compilation", optional_argument, 0, Tag::kCompilation },
    { L"lyrics", required_argument, 0, Tag::kLyrics },
    { L"artwork", required_argument, 0, Tag::kArtwork },
    { L"artwork-size", required_argument, 0, 'atsz' },
    { L"chapter", required_argument, 0, 'chap' },
    { L"tag", required_argument, 0, 'tag ' },
    { L"long-tag", required_argument, 0, 'ltag' },
    { 0, 0, 0, 0 }
};
static const uint32_t tag_keys[] = {
    Tag::kTitle,
    Tag::kSubTitle,
    Tag::kArtist,
    Tag::kAlbumArtist,
    Tag::kAlbum,
    Tag::kGrouping,
    Tag::kComposer,
    Tag::kComment,
    Tag::kGenre,
    Tag::kDate,
    Tag::kTrack,
    Tag::kDisk,
    Tag::kCompilation,
    Tag::kArtwork,
    Tag::kLyrics
};
const uint32_t * const tag_keys_end = tag_keys + array_size(tag_keys);

const char *get_qaac_version();

#ifdef REFALAC
#define PROGNAME "refalac"
#else
#define PROGNAME "qaac"
#endif

static
void usage()
{
    std::wprintf(L"%hs %hs\n%hs", PROGNAME, get_qaac_version(),
"Usage: " PROGNAME " [options] infiles....\n"
"\n"
"\"-\" as infile means stdin.\n"
#ifndef REFALAC
"On ADTS/WAV output mode, \"-\" as outfile means stdout.\n"
#endif
"\n"
"Main options:\n"
#ifndef REFALAC
"--formats              Show available AAC formats and exit\n"
"-a, --abr <bitrate>    AAC ABR mode / bitrate\n"
"-V, --tvbr <n>         AAC True VBR mode / quality [0-127]\n"
"-v, --cvbr <bitrate>   AAC Constrained VBR mode / bitrate\n"
"-c, --cbr <bitrate>    AAC CBR mode / bitrate\n"
"                       For -a, -v, -c, \"0\" as bitrate means \"highest\".\n"
"                       Highest bitrate available is automatically chosen.\n"
"                       For LC, default is -V90\n"
"                       For HE, default is -v0\n"
"--he                   HE AAC mode (TVBR is not available)\n"
"-q, --quality <n>      AAC encoding Quality [0-2]\n"
"--adts                 ADTS output (AAC only)\n"
"-A, --alac             ALAC encoding mode\n"
#else
"--fast                 Fast stereo encoding mode.\n"
#endif
"--check                Show library versions and exit\n"
"-D, --decode           Wave output mode.\n"
"--no-optimize          Don't optimize MP4 container file after encoding\n"
"-b, --bits-per-sample <n>\n"
"                       Bits per sample of output (for WAV/ALAC) [16/24]\n"
"-r, --rate <keep|auto|n>\n"
"                       keep: output sampling rate will be same as input\n"
"                             if possible.\n"
"                       auto: output sampling rate will be automatically\n"
"                             chosen by encoder.\n"
"                       n: desired output sampling rate in Hz\n"
"--lowpass <number>     Specify lowpass filter cut-off frequency in Hz\n"
"                       Use this when you want lower cut-off than\n"
"                       Apple default.\n"
"--gain <f>             Adjust gain by f dB.\n"
"                       Use negative value to decrese gain, when you want to\n"
"                       avoid clipping introduced by DSP.\n"
"-N, --normalize        Normalize (works in two pass. generates HUGE tempfile\n"
"                       for large input)\n"
"--delay <millisecs>    When positive value is given, prepend silence at the\n"
"                       begining to achieve delay of specified amount.\n"
"                       When negative value is given, specified length is\n"
"                       dropped from the beginning.\n"
"--matrix-preset <name> Specify preset remixing matrix name.\n"
"--matrix-file <file>   Specify file containing remixing matrix.\n"
"--no-matrix-normalize  Don't automatically normalize(scale) matrix\n"
"                       coefficients for the matrix mixer.\n"
"--chanmap <n1,n2...>   Re-arrange channels to the specified order.\n"
"                       For N-ch input, you take numbers 1,2..N, and\n"
"                       arrange them with comma-seperated, to the order\n"
"                       you want.\n"
"                       For example, \"--chanmap 2,1\" swaps left and right\n"
"--chanmask <n>         Force specified value as input channel mask(bitmap).\n"
"                       If --chanmask 0 is specified, qaac treats it as if\n"
"                       no channel mask is present in the source, and pick\n"
"                       default layout.\n"
"-d <dirname>           Output directory. Default is current working dir\n"
"--tmpdir <dirname>     Temporary directory. Default is %TMP%\n"
"-s, --silent           Suppress console messages\n"
"--verbose              More verbose console messages\n"
"-i, --ignorelength     Assume WAV input and ignore the data chunk length\n"
"-R, --raw              Raw PCM input\n"
"--threading            Enable multi-threading\n"
"-n, --nice             Give lower process priority\n"
"--text-codepage <n>    Specify text code page of cuesheet/chapter/lyrics.\n"
"                       1252 for Latin-1, 65001 for UTF-8.\n"
"                       Use this when automatic encoding detection fails.\n"
"-S, --stat             Save bitrate statistics into file\n"
"--log <filename>       Output message to file\n"
"\n"
"Option for single output file:\n"
"-o <filename>          Output filename\n"
"--concat               Encode whole inputs into single file. \n"
"                       Requires output filename (with -o)\n"
"\n"
"Option for cue sheet input:\n"
"--concat-cuesheet         Output single file rather than splitting by tracks\n"
"--fname-format <string>   Format string for output filename\n"
"\n"
"Options for Raw PCM input only:\n"
"--raw-channels <n>     Number of channels, default 2\n"
"--raw-rate     <n>     Sample rate, default 44100\n"
"--raw-format   <str>   Sample format, default S16L\n"
"                       Sample format spec:\n"
"                       1st char: S(igned) | U(nsigned) | F(loat)\n"
"                       2nd part: Bitwidth\n"
"                       Last part: L(ittle Endian) | B(ig Endian)\n"
"                       Cases are ignored. u8b is OK.\n"
"\n"
#ifndef REFALAC
"Options for CoreAudio sample rate converter:\n"
"--native-resampler[=line|norm|bats,n]\n"
"                       Arguments are optional.\n"
"                       Without argument, codec default SRC is used.\n"
"                       With argument, dedicated AudioConverter is used for\n"
"                       sample rate conversion.\n"
"                       '--native-resampler' and arguments must be delimited\n"
"                       by a '=' (space is not usable here).\n"
"                       Arguments must be delimited by a ','(comma).\n"
"                       First argument is sample rate converter complexity,\n"
"                       and one of line, norm, bats.\n"
"                         line: linear (worst, don't use this)\n"
"                         norm: normal\n"
"                         bats: mastering (best, but quite sloooow)\n"
"                       Second argument is sample rate converter quality,\n"
"                       which is an integer between 0-127.\n"
"                       Example:\n"
"                         --native-resampler\n"
"                         --native-resampler=norm,96\n"
"\n"
#endif
"Tagging options:\n"
" (same value is set to all files, so use with care for multiple files)\n"
"--title <string>\n"
"--subtitle <string>\n"
"--artist <string>\n"
"--band <string>       This means \"Album Artist\".\n"
"--album <string>\n"
"--grouping <string>\n"
"--composer <string>\n"
"--comment <string>\n"
"--genre <string>\n"
"--date <string>\n"
"--track <number[/total]>\n"
"--disk <number[/total]>\n"
"--compilation[=0|1]\n"
"                      By default, iTunes compilation flag is not set.\n"
"                      --compilation or --compilation=1 sets flag on.\n"
"                      --compilation=0 is same as default.\n"
"--lyrics <filename>\n"
"--artwork <filename>\n"
"--artwork-size <n>    Specify maximum width or height of artwork in pixels.\n"
"                      If specified artwork (with --artwork) is larger than\n"
"                      this, artwork is automatically resized.\n"
"--chapter <filename>\n"
"                      Set chapter from file.\n"
"--tag <fcc>:<value>\n"
"                      Set iTunes pre-defined tag with four-char-code key\n"
"                      and value.\n"
"                      1) For some fcc starting with U+00A9 (copyright sign),\n"
"                         you can enter 3 chars starting from the second char\n"
"                         instead.\n"
"                      2) Some known tags using type-code other than UTF-8\n"
"                         are taken care of. Rest are just written as UTF-8\n"
"                         string.\n"
"--long-tag <name>:<value>\n"
"                      Set long tag (iTunes custom metadata) with \n"
"                      arbitrary name/value pair. Value is always stored as\n"
"                      UTF8 string.\n"
    );
}

#ifndef REFALAC
static const wchar_t * const short_opts = L"hDo:d:b:r:insRSNAa:V:v:c:q:";
#else
static const wchar_t * const short_opts = L"hDo:d:b:r:insRSN";
#endif

bool Options::parse(int &argc, wchar_t **&argv)
{
    int ch, pos;
    while ((ch = wide::getopt_long(argc, argv,
				   short_opts, long_options, 0)) != EOF)
    {
	if (ch == 'h')
	    return usage(), false;
	else if (ch == 'chck')
	    this->check_only = true;
	else if (ch == 'fmts')
	    this->print_available_formats = true;
	else if (ch == 'o')
	    this->ofilename = wide::optarg;
	else if (ch == 'd')
	    this->outdir = wide::optarg;
	else if ((pos = strindex("cavV", ch)) >= 0) {
	    if ((this->output_format && !isAAC()) || this->method != -1) {
		std::fputws(L"Encoding mode options are exclusive.\n", stderr);
	    }
	    this->method = pos;
	    if (std::swscanf(wide::optarg, L"%u", &this->bitrate) != 1) {
		std::fputws(L"AAC Bitrate/Quality must be an integer.\n",
			    stderr);
		return false;
	    }
	}
	else if (ch == 'A') {
	    if ((this->output_format && !isALAC()) || this->method != -1) {
		std::fputws(L"Encoding mode options are exclusive.\n", stderr);
		return false;
	    }
	    this->output_format = 'alac';
	}
	else if (ch == 'D') {
	    if (this->output_format && !isLPCM()) {
		std::fputws(L"Encoding mode options are exclusive.\n", stderr);
		return false;
	    }
	    this->output_format = 'lpcm';
	}
	else if (ch == 'aach') {
	    if (this->output_format && !isAAC()) {
		std::fputws(L"--he is only available for AAC.\n", stderr);
		return false;
	    }
	    this->output_format = 'aach';
	}
	else if (ch == 'q') {
	    if (std::swscanf(wide::optarg, L"%u", &this->quality) != 1) {
		std::fputws(L"-q requires an integer.\n", stderr);
		return false;
	    }
	}
	else if (ch == 'log ')
	    this->logfilename = wide::optarg;
	else if (ch == 'nsrc') {
	    this->native_resampler = true;
	    if (wide::optarg) {
		std::vector<wchar_t> buffer(std::wcslen(wide::optarg) + 1);
		wchar_t *p = &buffer[0], *tok;
		std::wcscpy(p, wide::optarg);
		while (tok = wcssep(&p, L",")) {
		    int n;
		    if (std::swscanf(tok, L"%u", &n) == 1)
			this->native_resampler_quality = n;
		    else if (std::wcslen(tok) == 4)
			this->native_resampler_complexity =
			    fourcc(nallow(tok).c_str());
		    else {
			std::fputws(L"Invalid arg for --native-resampler.\n",
				    stderr);
			return false;
		    }
		}
	    }
	}
	else if (ch == 'N')
	    this->normalize = true;
	else if (ch == 's')
	    this->verbose = 0;
	else if (ch == 'verb')
	    this->verbose = 2;
	else if (ch == 'S')
	    this->save_stat = true;
	else if (ch == 'n')
	    this->nice = true;
	else if (ch == 'thrd')
	    this->threading = true;
	else if (ch == 'i')
	    this->ignore_length = true;
	else if (ch == 'R')
	    this->is_raw = true;
	else if (ch == 'ADTS')
	    this->is_adts = true;
	else if (ch == 'noop')
	    this->no_optimize = true;
	else if (ch == 'cat ')
	    this->concat = this->concat_cue = true;
	else if (ch == 'conc')
	    this->concat_cue = true;
	else if (ch == 'nfmt')
	    this->fname_format = wide::optarg;
	else if (ch == 'tmpd')
	    this->tmpdir = wide::optarg;
	else if (ch == 'nmxn')
	    this->no_matrix_normalize = true;
	else if (ch == 'cmap') {
	    std::vector<wchar_t> buff(std::wcslen(wide::optarg)+1);
	    wchar_t *bp = &buff[0], *tok;
	    std::wcscpy(bp, wide::optarg);
	    while ((tok = wcssep(&bp, L",")) != 0) {
		unsigned n;
		if (std::swscanf(tok, L"%u", &n) == 1)
		    this->chanmap.push_back(n);
		else {
		    std::fputws(L"Invalid arg for --chanmap.\n", stderr);
		    return false;
		}
	    }
	    uint32_t low = std::numeric_limits<int>::max();
	    uint32_t high = 0;
	    for (size_t i = 0; i < this->chanmap.size(); ++i) {
		uint32_t n = this->chanmap[i];
		if (n < low) low = n;
		if (n > high) high = n;
	    }
	    if (low < 1 || high > this->chanmap.size()) {
		std::fputws(L"Invalid arg for --chanmap.\n", stderr);
		return false;
	    }
	}
	else if (ch == 'r') {
	    if (!std::wcscmp(wide::optarg, L"keep"))
		this->rate = -1;
	    else if (!std::wcscmp(wide::optarg, L"auto"))
		this->rate = 0;
	    else if (std::swscanf(wide::optarg, L"%u", &this->rate) != 1) {
		std::fputws(L"Invalid arg for --rate.\n", stderr);
		return false;
	    }
	}
	else if (ch == 'mask') {
	    if (std::swscanf(wide::optarg, L"%i", &this->chanmask) != 1) {
		std::fputws(L"--chanmask requires an integer.\n", stderr);
		return false;
	    }
	}
	else if (ch == 'Rchn') {
	    if (std::swscanf(wide::optarg, L"%u", &this->raw_channels) != 1) {
		std::fputws(L"--raw-channels requires an integer.\n", stderr);
		return false;
	    }
	}
	else if (ch == 'Rrat') {
	    if (std::swscanf(wide::optarg, L"%u",
			     &this->raw_sample_rate) != 1) {
		std::fputws(L"--raw-rate requires an integer.\n", stderr);
		return false;
	    }
	}
	else if (ch == 'b') {
	    uint32_t n;
	    if (std::swscanf(wide::optarg, L"%u", &n) != 1
		|| (n != 16 && n != 24)) {
		std::fputws(L"Bits per sample shall be 16 or 24.\n", stderr);
		return false;
	    }
	    this->bits_per_sample = n;
	}
	else if (ch == 'Rfmt')
	    this->raw_format = wide::optarg;
	else if (ch == 'afst')
	    this->alac_fast = true;
	else if (ch == 'gain') {
	    if (std::swscanf(wide::optarg, L"%lf", &this->gain) != 1) {
		std::fputws(L"--gain requires an floating point number.\n",
			    stderr);
		return false;
	    }
	}
	else if (ch == 'lpf ') {
	    if (std::swscanf(wide::optarg, L"%u", &this->lowpass) != 1) {
		std::fputws(L"--lowpass requires an integer.\n", stderr);
		return false;
	    }
	}
	else if (ch == 'dlay') {
	    if (std::swscanf(wide::optarg, L"%d", &this->delay) != 1) {
		std::fputws(L"--delay requires an integer in millis.\n",
			    stderr);
		return false;
	    }
	}
	else if (ch == 'txcp') {
	    if (std::swscanf(wide::optarg, L"%u", &this->textcp) != 1) {
		std::fputws(L"--text-codepage requires code page number.\n",
			    stderr);
		return false;
	    }
	}
	else if (ch == 'atsz') {
	    if (std::swscanf(wide::optarg, L"%u", &this->artwork_size) != 1) {
		std::fputws(L"--artwork-size requires an integer.\n", stderr);
		return false;
	    }
	}
	else if (ch == Tag::kArtwork)
	    this->artworks.push_back(wide::optarg);
	else if (std::find(tag_keys, tag_keys_end, ch) != tag_keys_end) {
	    if (ch != Tag::kCompilation)
		this->tagopts[ch] = wide::optarg;
	    else if (!wide::optarg)
		this->tagopts[ch] = L"1";
	    else {
		int n;
		if (std::swscanf(wide::optarg, L"%d", &n) != 1) {
		    std::fputws(L"Invalid --compilation option arg.\n", stderr);
		    return false;
		}
		this->tagopts[ch] = wide::optarg;
	    }
	}
	else if (ch == 'tag ') {
	    std::vector<wchar_t> buff(std::wcslen(wide::optarg)+1);
	    wchar_t *value = &buff[0], *key;
	    std::wcscpy(value, wide::optarg);
	    key = wcssep(&value, L":");
	    size_t keylen = std::wcslen(key);
	    if (!value || (keylen != 3 && keylen != 4)) {
		std::fputws(L"Invalid --tag option arg.\n", stderr);
		return false;
	    }
	    uint32_t fcc = (keylen == 3) ? 0xa9 : 0;
	    wchar_t wc;
	    while ((wc = *key++) != 0) {
		if (wc != 0xa9 && (wc < 0x20 || wc > 0x7e)) {
		    std::fputws(L"Bogus fourcc for --tag.\n", stderr);
		    return false;
		}
		fcc = ((fcc << 8) | wc);
	    }
	    this->tagopts[fcc] = value;
	}
	else if (ch == 'ltag') {
	    std::vector<wchar_t> buff(std::wcslen(wide::optarg)+1);
	    wchar_t *value = &buff[0], *key;
	    std::wcscpy(value, wide::optarg);
	    key = wcssep(&value, L":");
	    if (!value) {
		std::fputws(L"Invalid arg for --long-tag.\n", stderr);
		return false;
	    }
	    this->longtags[w2m(key, utf8_codecvt_facet())] = value;
	}
	else if (ch == 'chap')
	    this->chapter_file = wide::optarg;
	else if (ch == 'mixp')
	    this->remix_preset = wide::optarg;
	else if (ch == 'mixm')
	    this->remix_file = wide::optarg;
	else
	    return false;
    }
    argc -= wide::optind;
    argv += wide::optind;

    if (!argc && !this->check_only && !this->print_available_formats) {
	if (wide::optind == 1)
	    return usage(), false;
	else {
	    std::fputws(L"Input file name is required.\n", stderr);
	    return false;
	}
    }
    if (argc > 1 && this->ofilename && !this->concat) {
	std::fputws(L"-o is not available for multiple output.\n", stderr);
	return false;
    }
    if (!this->output_format) {
#ifdef REFALAC
	this->output_format = 'alac';
#else
	this->output_format = 'aac ';
#endif
    }
    if (isSBR() && this->method == kTVBR) {
	std::fputws(L"TVBR is not available for HE.\n", stderr);
	return false;
    }
    if (isAAC() && this->method == -1) {
	this->method = isSBR() ? kCVBR : kTVBR;
	this->bitrate = isSBR() ? 0 : 90;
    }
    if (isMP4() && this->ofilename && !std::wcscmp(this->ofilename, L"-")) {
	std::fputws(L"MP4 piping is not supported.\n", stderr);
	return false;
    }
    if (!isAAC() && this->is_adts) {
	std::fputws(L"--adts is only available for AAC.\n", stderr);
	return false;
    }
    if (!isAAC() && this->quality != -1) {
	std::fputws(L"-q is only available for AAC.\n", stderr);
	return false;
    }
    if (this->ignore_length && this->is_raw) {
	std::fputws(L"Can't use --ignorelength and --raw at the same time.\n",
		    stderr);
	return false;
    }
    if (this->concat && argc > 1 && !this->ofilename) {
	std::fputws(L"--concat requires output filename (use -o option).\n",
		    stderr);
	return false;
    }
    if (this->quality == -1)
	this->quality = 2;
    return true;
}
