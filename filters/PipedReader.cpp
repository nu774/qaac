#include "PipedReader.h"

namespace {
    const int NSAMPLES = 0x1000;
    const int PIPE_BUF_FACTOR = 4;
}

PipedReader::PipedReader(std::shared_ptr<ISource> &src):
    FilterBase(src), m_thread(0), m_position(0)
{
    HANDLE hr, hw;
    int fd;
    FILE *fp;

    uint32_t bpf = src->getSampleFormat().mBytesPerFrame;
    if (!CreatePipe(&hr, &hw, 0, NSAMPLES * bpf * PIPE_BUF_FACTOR))
        win32::throw_error("CreatePipe", GetLastError());
    CHECKCRT((fd = _open_osfhandle(reinterpret_cast<intptr_t>(hr),
                                   _O_RDONLY|_O_BINARY)) < 0);
    CHECKCRT((fp = _fdopen(fd, "rb")) == 0);
    m_readPipe.reset(fp, std::fclose);
    m_writePipe.reset(hw, CloseHandle);
}

PipedReader::~PipedReader()
{
    if (m_thread.get()) {
        /*
         * Let InputThread quit if it's still running.
         * (WriteFile() will immediately fail, even if it is blocking on it)
         */
        m_readPipe.reset();
        WaitForSingleObject(m_thread.get(), INFINITE);
    }
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
        HANDLE ph = m_writePipe.get();
        size_t n;
        DWORD nb;
        while ((n = src->readSamples(bp, NSAMPLES)) > 0
               && WriteFile(ph, bp, n * bpf, &nb, 0))
            ;
    } catch (...) {}
    m_writePipe.reset(); // close
}
