#include "IInputStream.h"
#include "win32util.h"
#include <vector>

class SeekableInputStream: public IInputStream {
public:
    SeekableInputStream(const std::string &path);
    ~SeekableInputStream();
    bool seekable() override { return m_seekable; }
    int read(void *buf, unsigned size) override;
    int64_t seek(int64_t off, int whence) override;
    int64_t tell() override { return m_pos; }
    int64_t size() override { return m_size; }
private:
    SeekableInputStream(const SeekableInputStream&);
    SeekableInputStream & operator=(const SeekableInputStream &);

    void fillBuffer();
    void clearBuffer();
    int64_t seekRaw(int64_t pos);
    int64_t bufferPos() { return m_fd_pos - m_buffer.size(); }
    int readPosInBuffer() { return m_pos - m_fd_pos + m_buffer.size(); }

    int64_t m_pos;            // logical position in the stream
    int64_t m_fd_pos;     // position of m_fd in the file
    bool m_eof;
    int m_fd;
    int64_t m_size;
    bool m_seekable;
    std::vector<uint8_t> m_buffer;
};
