#ifndef PIPED_READER_H
#define PIPED_READER_H

#include "FilterBase.h"
#include "win32util.h"
#include <thread>

class PipedReader: public FilterBase {
    std::shared_ptr<FILE> m_readPipe;
#ifdef _WIN32
    std::shared_ptr<void> m_writePipe;
#else
    int m_writeFd;
#endif
    std::thread m_thread;
    int64_t m_position;
public:
    PipedReader(std::shared_ptr<ISource> &src);
    ~PipedReader();
    size_t readSamples(void *buffer, size_t nsamples);
    void start()
    {
        m_thread = std::thread(&PipedReader::inputThreadProc, this);
    }
    int64_t getPosition() { return m_position; }
private:
    void inputThreadProc();
};

#endif
