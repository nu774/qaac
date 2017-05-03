#ifndef PIPED_READER_H
#define PIPED_READER_H

#include "FilterBase.h"
#include "win32util.h"
#include <process.h>

class PipedReader: public FilterBase {
    std::shared_ptr<FILE> m_readPipe;
    std::shared_ptr<void> m_writePipe, m_thread;
    int64_t m_position;
public:
    PipedReader(std::shared_ptr<ISource> &src);
    ~PipedReader();
    size_t readSamples(void *buffer, size_t nsamples);
    void start()
    {
        intptr_t h = _beginthreadex(0, 0, staticInputThreadProc, this, 0, 0);
        if (h == -1)
            throw std::runtime_error(std::strerror(errno));
        m_thread.reset(reinterpret_cast<HANDLE>(h), CloseHandle);
    }
    int64_t getPosition() { return m_position; }
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
