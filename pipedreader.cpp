#include "pipedreader.h"

PipedReader::PipedReader(x::shared_ptr<ISource> &src):
    DelegatingSource(src), m_thread(0), m_quitFlag(0)
{
    HANDLE hr, hw;
    if (!CreatePipe(&hr, &hw, 0, 0x8000))
	throw_win32_error("CreatePipe", GetLastError());
    m_readPipe.reset(hr, CloseHandle);
    m_writePipe.reset(hw, CloseHandle);
}

PipedReader::~PipedReader()
{
    if (m_thread) {
	InterlockedExchange(&m_quitFlag, 1);
	WaitForSingleObject(m_thread, INFINITE);
	CloseHandle(m_thread);
    }
}

size_t PipedReader::readSamples(void *buffer, size_t nsamples)
{
    uint32_t bpf = source()->getSampleFormat().bytesPerFrame();
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
    return (bp - static_cast<uint8_t*>(buffer)) / bpf;
}

void PipedReader::inputThreadProc()
{
    ISource *src = source();
    uint32_t bpf = src->getSampleFormat().bytesPerFrame();
    std::vector<uint8_t> buffer(4096 * bpf);
    for (;;) {
	if (InterlockedCompareExchange(&m_quitFlag, 2, 1) == 1)
	    break;
	size_t nsamples = src->readSamples(&buffer[0], 4096);
	if (nsamples == 0)
	    break;
	DWORD written;
	WriteFile(m_writePipe.get(), &buffer[0], nsamples * bpf, &written, 0);
    }
    m_writePipe.reset(); // close
}
