#ifndef PIPED_READER_H
#define PIPED_READER_H

#include "iointer.h"
#include "win32util.h"
#include <process.h>

class PipedReader: public DelegatingSource {
    x::shared_ptr<void> m_readPipe, m_writePipe;
    HANDLE m_thread;
    volatile LONG m_quitFlag;
public:
    PipedReader(x::shared_ptr<ISource> &src);
    ~PipedReader();
    size_t readSamples(void *buffer, size_t nsamples);
    void start()
    {
	intptr_t h = _beginthreadex(0, 0, staticInputThreadProc, this, 0, 0);
	if (h == -1)
	    throw std::runtime_error(std::strerror(errno));
	m_thread = reinterpret_cast<HANDLE>(h);
    }
private:
    void inputThreadProc();
    static unsigned __stdcall staticInputThreadProc(void *arg)
    {
	PipedReader *self = static_cast<PipedReader*>(arg);
	self->inputThreadProc();
	return 0;
    }
};

#endif
