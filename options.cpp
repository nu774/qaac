#include "options.h"
#include "getopt.h"
#include "itunetags.h"

static struct option long_options[] = {
    { L"help", no_argument, 0, 'h' },
    { L"check", no_argument, 0, 'chck' },
    { L"formats", no_argument, 0, 'fmts' },
    { L"abr", required_argument, 0, 'a' },
    { L"tvbr", required_argument, 0, 'V' },
    { L"cvbr", required_argument, 0, 'v' },
    { L"cbr", required_argument, 0, 'c' },
    { L"he", no_argument, 0, 'aach' },
    { L"alac", no_argument, 0, 'A' },
    { L"quality", required_argument, 0, 'q' },
    { L"rate", required_argument, 0, 'r' },
    { L"silent", no_argument, 0, 's' },
    { L"stat", no_argument, 0, 'S' },
    { L"nice", no_argument, 0, 'n' },
    { L"native-chanmapper", no_argument, 0, 'nchm' },
    { L"chanmap", required_argument, 0, 'cmap' },
    { L"downmix", required_argument, 0, 'dmix' },
    { L"chanmask", required_argument, 0, 'mask' },
    { L"no-optimize", no_argument, 0, 'noop' },
    { L"native-resampler", no_argument, 0, 'nsmp' },
    { L"normalize", no_argument, 0, 'N' },
    { L"raw", no_argument, 0, 'R' },
    { L"raw-channels", required_argument, 0,  'Rchn' },
    { L"raw-rate", required_argument, 0,  'Rrat' },
    { L"raw-format", required_argument, 0,  'Rfmt' },
    { L"adts", no_argument, 0, 'ADTS' },
    { L"ignorelength", no_argument, 0, 'i' },
    { L"fname-format", required_argument, 0, 'nfmt' },
    { L"log", required_argument, 0, 'log ' },
    { L"title", required_argument, 0, Tag::kTitle },
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
    { L"compilation", no_argument, 0, Tag::kCompilation },
    { L"artwork", required_argument, 0, Tag::kArtwork },
    { 0, 0, 0, 0 }
};
static const uint32_t tag_keys[] = {
    Tag::kTitle,
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
};
const uint32_t * const tag_keys_end = tag_keys + array_size(tag_keys);

const char *get_qaac_version();

static
void usage()
{
    std::printf("qaac %s\n", get_qaac_version());
    std::puts(
"Usage: qaac [options] infiles....\n"
"\n"
"\"-\" as infile means stdin.\n"
"In ADTS output mode, \"-\" as outfile means stdout.\n"
"\n"
"Main options:\n"
"--check                Show QT version etc, and exit\n"
"--formats              Print available AAC formats, and exit\n"
"-d <dirname>           Output directory, default is cwd\n"
"-a, --abr <bitrate>    AAC ABR mode / bitrate\n"
"-V, --tvbr <n>         AAC True VBR mode / quality [0-127]\n"
"-v, --cvbr <bitrate>   AAC Constrained VBR mode / bitrate\n"
"-c, --cbr <bitrate>    AAC CBR mode / bitrate\n"
"                       For -a, -v, -c, \"0\" as bitrate means \"highest\".\n"
"                       Highest bitrate available is automatically chosen.\n"
"                       For LC, default is -V90\n"
"                       For HE, default is -v0\n"
"--he                   HE AAC mode (Can't use TVBR)\n"
"-A, --alac             ALAC encoding mode\n"
"-q, --quality <n>      AAC encoding Quality [0-2]\n"
"-r, --rate <option>    Sample rate option (AAC only)\n"
"                       Specify one of the followings:\n"
"                       keep: Try to preserve the original rate\n"
"                       auto: Let QuickTime choose the optimal one\n"
"                       <number>: Literal rate in Hz\n"
"-s, --silent           Don't be verbose\n"
"-n, --nice             Give lower process priority\n"
"--native-chanmapper    Use QuickTime native channel mapper\n"
"                       Don't set this when using buggy versions\n"
"--chanmap <n1,n2...>   Remap channel order\n"
"                       For Nch input, you take numbers 1,2..N, and\n"
"                       re-order them with comma seperated, to the order\n"
"                       you want.\n"
"                       For example, \"--chanmap 2,1\" swaps left and right\n"
"--downmix <mono|stereo>    Downmix to mono/stereo\n"
"--chanmask <n>         Force specified value as input channel mask(bitmap).\n"
"                       If --chanmask 0 is specified, qaac treats it as if\n"
"                       no channel mask is present in the source, and pick\n"
"                       default layout.\n"
"--no-optimize          Don't optimize MP4 container file after encoding\n"
"--native-resampler     Always use QuickTime built-in resampler\n"
"-N, --normalize        Normalize after resample (works in two-pass)\n"
"                       Normalize is done only when rate conversion occurs\n"
"--adts                 ADTS(raw AAC)output, instead of m4a(AAC only)\n"
"-i, --ignorelength     Assume WAV input and ignore the data chunk length\n"
"-R, --raw              Raw PCM input\n"
"-S, --stat             Save bitrate statistics into file\n"
"--log <filename>       Output message to file\n"
"\n"
"Option for single input mode only:\n"
"-o <filename>          Output filename\n"
"\n"
"Option for cue sheet input:\n"
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
"Tagging options:\n"
" (same value is set to all files, so use with care for multiple files)\n"
"--title <string>\n"
"--artist <string>\n"
"--band <string>\n"
"--album <string>\n"
"--grouping <string>\n"
"--composer <string>\n"
"--comment <string>\n"
"--genre <string>\n"
"--date <string>\n"
"--track <number[/total]>\n"
"--disk <number[/total]>\n"
"--compilation\n"
"--artwork <filename>\n"
    );
}

bool Options::parse(int &argc, wchar_t **&argv)
{
    int ch, pos;
    while ((ch = getopt_long(argc, argv, L"hAo:d:a:V:v:c:q:r:insRSN",
		    long_options, 0)) != EOF)
    {
	if (ch == 'h')
	    return usage(), false;
	else if (ch == 'chck')
	    this->check_only = true;
	else if (ch == 'fmts')
	    this->print_available_formats = true;
	else if (ch == 'o')
	    this->ofilename = optarg;
	else if (ch == 'd')
	    this->outdir = optarg;
	else if (ch == 'log ')
	    this->logfilename = optarg;
	else if (ch == 'A') {
	    if ((this->output_format && !isALAC()) || this->method != -1)
		return usage(), false;
	    this->output_format = 'alac';
	}
	else if (ch == 'aach') {
	    if (this->output_format && !isAAC())
		return usage(), false;
	    this->output_format = 'aach';
	}
	else if (ch == 'q') {
	    if (std::swscanf(optarg, L"%u", &this->quality) != 1) {
		std::fputs("Quality value must be an integer\n", stderr);
		return false;
	    }
	}
	else if (ch == 'nsmp')
	    this->native_resampler = true;
	else if (ch == 'N')
	    this->normalize = true;
	else if (ch == 's')
	    this->verbose = false;
	else if (ch == 'S')
	    this->save_stat = true;
	else if (ch == 'n')
	    this->nice = true;
	else if (ch == 'nchm')
	    this->native_chanmapper = true;
	else if (ch == 'i')
	    this->ignore_length = true;
	else if (ch == 'R')
	    this->is_raw = true;
	else if (ch == 'ADTS')
	    this->is_adts = true;
	else if (ch == 'noop')
	    this->no_optimize = true;
	else if (ch == 'nfmt')
	    this->fname_format = optarg;
	else if (ch == 'cmap') {
	    std::vector<wchar_t> buff(wcslen(optarg)+1);
	    wchar_t *bp = &buff[0], *tok;
	    std::wcscpy(bp, optarg);
	    while ((tok = wcssep(&bp, L",")) != 0) {
		unsigned n;
		if (std::swscanf(tok, L"%u", &n) == 1)
		    this->chanmap.push_back(n);
		else 
		    return usage(), false;
	    }
	    uint32_t low = INT_MAX;
	    uint32_t high = 0;
	    for (size_t i = 0; i < this->chanmap.size(); ++i) {
		uint32_t n = this->chanmap[i];
		if (n < low) low = n;
		if (n > high) high = n;
	    }
	    if (low < 1 || high > this->chanmap.size()) {
		std::fputs("Invalid channel mapping spec\n", stderr);
		return false;
	    }
	}
	else if (ch == 'dmix') {
	    if (!std::wcscmp(optarg, L"mono"))
		this->downmix = 1;
	    else if (!std::wcscmp(optarg, L"stereo"))
		this->downmix = 2;
	    else {
		std::fputs("Only \"mono\" or \"stereo\""
			" is allowed for downmixing\n", stderr);
		return false;
	    }
	}
	else if (ch == 'r') {
	    if (!std::wcscmp(optarg, L"auto"))
		this->rate = 0;
	    else if (!std::wcscmp(optarg, L"keep"))
		this->rate = -1;
	    else if (std::swscanf(optarg, L"%u", &this->rate) != 1) {
		std::fputs("Invalid rate value\n", stderr);
		return false;
	    }
	}
	else if (ch == 'mask') {
	    if (std::swscanf(optarg, L"%i", &this->chanmask) != 1) {
		std::fputs("Integer required for channel mask.\n", stderr);
		return false;
	    }
	}
	else if (ch == 'Rchn') {
	    if (std::swscanf(optarg, L"%u", &this->raw_channels) != 1) {
		std::fputs("Raw channels must be an integer\n", stderr);
		return false;
	    }
	}
	else if (ch == 'Rrat') {
	    if (std::swscanf(optarg, L"%u", &this->raw_sample_rate) != 1) {
		std::fputs("Raw sample rate must be an integer\n", stderr);
		return false;
	    }
	}
	else if (ch == 'Rfmt')
	    this->raw_format = optarg;
	else if ((pos = strindex("aVvc", ch)) >= 0) {
	    if ((this->output_format && !isAAC()) || this->method != -1)
		return usage(), false;
	    this->method = pos;
	    if (std::swscanf(optarg, L"%u", &this->bitrate) != 1) {
		std::fputs("AAC Bitrate/Quality must be an integer\n", stderr);
		return false;
	    }
	}
	else if (ch == Tag::kArtwork)
	    this->artworks.push_back(optarg);
	else if (std::find(tag_keys, tag_keys_end, ch) != tag_keys_end)
	    this->tagopts[ch]
		= (ch == Tag::kCompilation) ? L"1" : optarg;
	else
	    return false;
    }
    argc -= optind;
    argv += optind;

    if (!argc && !this->check_only && !this->print_available_formats)
	return usage(), false;
    if (argc > 1 && (this->ofilename || this->tagopts.size())) {
	this->ofilename = 0;
	this->tagopts.clear();
    }
    if (!this->output_format)
	this->output_format = 'aac ';
    if (isSBR() && this->method == kTVBR) {
	std::fputs("Can't use TVBR method in HE encoding mode\n", stderr);
	return false;
    }
    if (isAAC() && this->method == -1) {
	this->method = isSBR() ? kCVBR : kTVBR;
	this->bitrate = isSBR() ? 0 : 90;
    }
    if (!isAAC() && this->is_adts) {
	std::fputs("ADTS output mode is only allowed for AAC encoding\n",
		stderr);
	return false;
    }
    if (isALAC() && this->quality != -1) {
	std::fputs("Quality is only available for AAC\n", stderr);
	return false;
    }
    if (this->quality == -1)
	this->quality = 2;
    return true;
}
