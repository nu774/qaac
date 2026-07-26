#include "PipedReader.h"
#ifndef _WIN32
#include <unistd.h>
#endif

namespace {
    const int NSAMPLES = 0x1000;
    const int PIPE_BUF_FACTOR = 4;
}

PipedReader::PipedReader(std::shared_ptr<ISource> &src):
    FilterBase(src),
#ifndef _WIN32
    m_writeFd(-1),
#endif
    m_position(0)
{
    uint32_t bpf = src->getSampleFormat().mBytesPerFrame;
#ifdef _WIN32
    HANDLE hr, hw;
    int fd;
    FILE *fp;
    if (!CreatePipe(&hr, &hw, 0, NSAMPLES * bpf * PIPE_BUF_FACTOR))
        win32::throw_error("CreatePipe", GetLastError());
    CHECKCRT((fd = _open_osfhandle(reinterpret_cast<intptr_t>(hr),
                                   _O_RDONLY|_O_BINARY)) < 0);
    CHECKCRT((fp = _fdopen(fd, "rb")) == 0);
    m_readPipe.reset(fp, std::fclose);
    m_writePipe.reset(hw, CloseHandle);
#else
    int fds[2];
    if (pipe(fds) != 0)
        util::throw_crt_error("pipe");
    FILE *fp = fdopen(fds[0], "rb");
    if (!fp) util::throw_crt_error("fdopen");
    m_readPipe.reset(fp, std::fclose);
    m_writeFd = fds[1];
#endif
}

PipedReader::~PipedReader()
{
    if (m_thread.joinable()) {
        m_readPipe.reset();
        m_thread.join();
    }
#ifndef _WIN32
    if (m_writeFd >= 0)
        close(m_writeFd);
#endif
}

size_t PipedReader::readSamples(void *buffer, size_t nsamples)
{
    uint32_t bpf = source()->getSampleFormat().mBytesPerFrame;
    ssize_t nread = 0;
    if (m_readPipe.get()) {
        nread = util::nread(fileno(m_readPipe.get()), buffer, nsamples * bpf);
        if (nread == 0)
            m_readPipe.reset();
    }
    nsamples = nread / bpf;
    m_position += nsamples;
    return nsamples;
}

void PipedReader::inputThreadProc()
{
    try {
        ISource *src = source();
        uint32_t bpf = src->getSampleFormat().mBytesPerFrame;
        std::vector<uint8_t> buffer(NSAMPLES * bpf);
        uint8_t *bp = &buffer[0];
        size_t n;
#ifdef _WIN32
        HANDLE ph = m_writePipe.get();
        DWORD nb;
        while ((n = src->readSamples(bp, NSAMPLES)) > 0
               && WriteFile(ph, bp, n * bpf, &nb, 0))
            ;
#else
        while ((n = src->readSamples(bp, NSAMPLES)) > 0) {
            size_t nbytes = n * bpf;
            size_t off = 0;
            bool ok = true;
            while (off < nbytes) {
                ssize_t w = write(m_writeFd, bp + off, nbytes - off);
                if (w <= 0) { ok = false; break; }
                off += static_cast<size_t>(w);
            }
            if (!ok) break;
        }
#endif
    } catch (...) {}
#ifdef _WIN32
    m_writePipe.reset(); // close
#else
    if (m_writeFd >= 0) { close(m_writeFd); m_writeFd = -1; }
#endif
}
