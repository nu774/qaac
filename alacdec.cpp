#include <iostream>
#include <cstdlib>
#include <clocale>
#include <io.h>
#include <fcntl.h>
#define NOMINMAX
#include <windows.h>
#include <shellapi.h>
#include <GNUCompatibility/stdint.h> // To avoid conflict with QT
#include "impl.h"
#include "getopt.h"
#include "qtmoviesource.h"
#include "wavsink.h"
#include "flacsink.h"

void usage()
{
    std::cerr <<
"Usage: alacdec [-o OUTFILE | -d OUTDIR | -F] INFILE...\n"
"-o <file>        Output filename (single input only)\n"
"-d <dirname>     Output directory name\n"
"-F               FLAC output\n"
    << std::flush;
    std::exit(1);
}

void noop(void *) {}

void decode(const wchar_t *ifile, const wchar_t *ofile, const wchar_t *odir,
	bool flac=false)
{
    QTMovieSource source(ifile);
    const SampleFormat &fmt = source.getSampleFormat();

    std::wstring ofilename;
    const wchar_t *ext = flac ? L"flac" : L"wav";
    if (ofile)
	ofilename = ofile;
    else
	ofilename = std::wstring(odir) + L"/" +
		PathReplaceExtension(PathFindFileNameW(ifile), ext);

    typedef x::shared_ptr<FILE> file_ptr_t;
    file_ptr_t ofp;
    if (!std::wcscmp(ofilename.c_str(), L"-")) {
	_setmode(1, _O_BINARY);
	ofp = file_ptr_t(stdout, noop);
    } else {
	FILE *fp = wfopenx(ofilename.c_str(), L"wb");
	ofp = file_ptr_t(fp, std::fclose);
    }
    ISink *sink = 0;
    if (flac) {
	SetDllDirectoryW(L"");
	FLACModule libflac(L"libFLAC.dll");
	if (!libflac.loaded())
	    throw std::runtime_error("libflac is not loaded");
	sink = new FLACSink(ofp.get(), source.length(), fmt,
		libflac, source.getTags());
    } else
	sink = new WavSink(ofp.get(), source.length(), fmt);
    std::auto_ptr<ISink> __delete_later__(sink);

    const size_t nsamples = 0x1000;
    std::vector<char> buffer(nsamples * fmt.bytesPerFrame());
    size_t len;
    uint64_t processed = 0;
    while ((len = source.readSamples(&buffer[0], nsamples)) > 0) {
	sink->writeSamples(&buffer[0], len * fmt.bytesPerFrame(), len);
	processed += len;
	std::cerr << format("\r%" PRId64 "/%" PRId64 " samples processed",
		processed, source.length());
    }
    std::cerr << std::endl;
}

#ifdef _MSC_VER
int wmain(int argc, wchar_t **argv)
#else
int wmain1(int argc, wchar_t **argv)
#endif
{
#ifdef DEBUG_ATTACH
    std::getchar();
#endif

    std::setbuf(stderr, 0);
    std::setlocale(LC_CTYPE, "");

    const wchar_t *ofile = 0;
    const wchar_t *odir = L".";
    bool flac = false;
    int ch;
    while ((ch = getopt(argc, argv, L"o:d:F")) != EOF) {
	if (ch == 'o')
	    ofile = optarg;
	else if (ch == 'd')
	    odir = optarg;
	else if (ch == 'F')
	    flac = true;
	else
	    return 1;
    }
    argc -= optind;
    argv += optind;
    if (!argc || (argc > 1 && ofile))
	usage();


    std::cerr << "initializing QTML..." << std::flush;
    QTInitializer __quicktime__;
    std::cerr << "done" << std::endl;

    mp4v2::impl::log.setVerbosity(MP4_LOG_NONE);
    try {
	const wchar_t *ifile;
	while ((ifile = *argv++)) {
	    wchar_t *f = PathFindFileNameW(ifile);
	    std::cerr << format("\n%ls\n", f);
	    decode(ifile, ofile, odir, flac);
	}
    } catch (const std::runtime_error &e) {
	std::cerr << std::endl << e.what() << std::endl;
	return 2;
    }
    return 0;
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
