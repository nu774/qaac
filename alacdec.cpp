#include <iostream>
#include <cstdlib>
#include <clocale>
#include <io.h>
#include <fcntl.h>
#include "getopt.h"
#include "qtmoviesource.h"
#include "wavsink.h"

void usage()
{
    std::cerr <<
"Usage: alacdec [-o OUTFILE | -d OUTDIR] INFILE...\n"
"-o option can be specified only when single input mode\n";
    std::exit(1);
}

void noop(void *) {}

void decode(const wchar_t *ifile, const wchar_t *ofile, const wchar_t *odir)
{
    QTMovieSource source(ifile, true);
    const SampleFormat &fmt = source.getSampleFormat();

    std::wstring ofilename;
    if (ofile)
	ofilename = ofile;
    else
	ofilename = std::wstring(odir) + L"/" +
		PathReplaceExtension(PathFindFileNameW(ifile), L"wav");

    typedef std::tr1::shared_ptr<FILE> file_ptr_t;
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
    WavSink sink(ofp.get(), source.length(), fmt, source.getChannelMap());

    const size_t nsamples = 0x1000;
    std::vector<char> buffer(nsamples * fmt.bytesPerFrame());
    size_t len;
    uint64_t processed = 0;
    while ((len = source.readSamples(&buffer[0], nsamples)) > 0) {
	sink.writeSamples(&buffer[0], len * fmt.bytesPerFrame(), len);
	processed += len;
	std::cerr << format("\r" LL "d/" LL "d samples processed",
		processed, source.length());
    }
    std::cerr << std::endl;
}

int wmain(int argc, wchar_t **argv)
{
    std::setbuf(stderr, 0);

    const wchar_t *ofile = 0;
    const wchar_t *odir = L".";
    int ch;
    while ((ch = getopt(argc, argv, L"o:d:")) != EOF) {
	if (ch == 'o')
	    ofile = optarg;
	else if (ch == 'd')
	    odir = optarg;
	else
	    return 1;
    }
    argc -= optind;
    argv += optind;
    if (!argc || (argc > 1 && ofile))
	usage();

    SetPriorityClass(GetCurrentProcess(), IDLE_PRIORITY_CLASS);
    QTInitializer __quicktime__(true);
    try {
	const wchar_t *ifile;
	while (ifile = *argv++) {
	    std::cerr << format("\n%ls\n", PathFindFileNameW(ifile));
	    decode(ifile, ofile, odir);
	}
    } catch (const std::runtime_error &e) {
	std::cerr << std::endl << e.what() << std::endl;
	return 2;
    }
    return 0;
}
