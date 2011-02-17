#include <iostream>
#include <cstdlib>
#include <clocale>
#include <io.h>
#include <fcntl.h>
#include <stdint.h>
#include "impl.h"
#include "getopt.h"
#include "qtmoviesource.h"
#include "wavsink.h"
#include "flacsink.h"

void usage()
{
    std::cerr <<
"Usage: alacdec [-o OUTFILE | -d OUTDIR | -F] INFILE...\n"
"-o option can be specified only when single input mode\n";
    std::exit(1);
}

void noop(void *) {}

void decode(const wchar_t *ifile, const wchar_t *ofile, const wchar_t *odir,
	bool flac=false)
{
    QTMovieSource source(ifile, true);
    const SampleFormat &fmt = source.getSampleFormat();

    std::wstring ofilename;
    const wchar_t *ext = flac ? L"flac" : L"wav";
    if (ofile)
	ofilename = ofile;
    else
	ofilename = std::wstring(odir) + L"/" +
		PathReplaceExtension(PathFindFileNameW(ifile), ext);

    typedef boost::shared_ptr<FILE> file_ptr_t;
    file_ptr_t ofp;
    if (!std::wcscmp(ofilename.c_str(), L"-")) {
	_setmode(1, _O_BINARY);
	ofp.swap(file_ptr_t(stdout, noop));
    } else {
	FILE *fp = _wfopen(ofilename.c_str(), L"wb");
	if (!fp)
	    throw std::runtime_error(std::strerror(errno));
	ofp.swap(file_ptr_t(fp, std::fclose));
    }
    ISink *sink = 0;
    if (flac) {
	std::wstring selfpath = GetModuleFileNameX();
	const wchar_t *fpos = PathFindFileNameW(selfpath.c_str());
	std::wstring selfdir = selfpath.substr(0, fpos - selfpath.c_str());
	FLACModule libflac(selfdir + L"libFLAC_vc10.dll");
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

int wmain(int argc, wchar_t **argv)
{
#ifdef DEBUG_ATTACH
    std::getchar();
#endif

    std::setbuf(stderr, 0);

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

    QTInitializer __quicktime__(true);
    try {
	const wchar_t *ifile;
	while (ifile = *argv++) {
	    std::cerr << format("\n%ls\n", PathFindFileNameW(ifile));
	    decode(ifile, ofile, odir, flac);
	}
    } catch (const std::runtime_error &e) {
	std::cerr << std::endl << e.what() << std::endl;
	return 2;
    }
    return 0;
}
