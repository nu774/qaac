#include "pipedreader.h"

PipedReader::PipedReader(std::shared_ptr<ISource> &src):
    FilterBase(src), m_thread(0), m_position(0)
{
    HANDLE hr, hw;
    if (!CreatePipe(&hr, &hw, 0, 0x8000))
	win32::throw_error("CreatePipe", GetLastError());
    m_readPipe.reset(hr, CloseHandle);
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
    uint8_t *bp = static_cast<uint8_t*>(buffer);
    size_t count = nsamples * bpf;
    while (count > 0) {
	DWORD nread = 0;
	ReadFile(m_readPipe.get(), bp, count, &nread, 0);
	count -= nread;
	bp += nread;
	if (nread == 0) {
	    m_readPipe.reset();
	    break;
	}
    }
    nsamples = (bp - static_cast<uint8_t*>(buffer)) / bpf;
    m_position += nsamples;
    return nsamples;
}

void PipedReader::inputThreadProc()
{
    try {
	ISource *src = source();
	uint32_t bpf = src->getSampleFormat().mBytesPerFrame;
	std::vector<uint8_t> buffer(4096 * bpf);
	uint8_t *bp = &buffer[0];
	HANDLE ph = m_writePipe.get();
	size_t n;
	DWORD nb;
	while ((n = src->readSamples(bp, 4096)) > 0
	       && WriteFile(ph, bp, n * bpf, &nb, 0))
	    ;
    } catch (...) {}
    m_writePipe.reset(); // close
}
