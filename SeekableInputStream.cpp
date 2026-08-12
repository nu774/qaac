#include "SeekableInputStream.h"
#include "strutil.h"
#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#include <fcntl.h>
#endif
#include <sys/stat.h>

SeekableInputStream::SeekableInputStream(const std::string &path)
    : m_pos(0)
    , m_fd_pos(0)
    , m_eof(false)
    , m_seekable(false)
    , m_size(-1)
{
    if (path == "-")
        m_fd = fileno(stdin);
    else {
#ifdef _WIN32
        m_fd = _wsopen(strutil::us2w(path).c_str(), _O_RDONLY|_O_BINARY, _SH_DENYWR);
#else
        m_fd = open(path.c_str(), O_RDONLY);
#endif
    }
    if (m_fd == -1) {
        util::throw_crt_error(path);
    }
#ifdef _WIN32
    struct _stat64 stb = { 0 };
    if (_fstat64(m_fd, &stb) == 0 && (stb.st_mode & _S_IFMT) == _S_IFREG) {
        m_size = stb.st_size;
        m_seekable = true;
    }
#else
    struct stat stb = { 0 };
    if (fstat(m_fd, &stb) == 0 && S_ISREG(stb.st_mode)) {
        m_size = stb.st_size;
        m_seekable = true;
    }
#endif
    m_buffer.reserve(0x800000); // 8MiB
}

SeekableInputStream::~SeekableInputStream()
{
    if (m_fd != 0)
        close(m_fd);
}

int SeekableInputStream::read(void *buf, unsigned size)
{
    uint8_t *p = static_cast<uint8_t*>(buf);
    unsigned nc = 0;
    while (nc < size) {
        if (!m_eof && (m_buffer.empty() || m_pos >= m_fd_pos)) {
            fillBuffer();
        }
        if (m_buffer.empty() || m_fd_pos == m_pos) break;
        int count = std::min((int)(size - nc), (int)(m_fd_pos - m_pos));
        if (count > 0) {
            std::memcpy(p, m_buffer.data() + readPosInBuffer(), count);
            nc += count;
            p += count;
            m_pos += count;
        }
    }
    return nc;
}

int64_t SeekableInputStream::seek(int64_t off, int whence)
{
    if (whence == SEEK_CUR) {
        off += m_pos;
    } else if (whence == SEEK_END) {
        if (m_size < 0) return -1;
        off += m_size;
    }
    if (!m_buffer.empty() && off >= bufferPos() && off <= m_fd_pos) {
        m_pos = off;
        return m_pos;
    }
    if (!m_seekable) {
        if (off < bufferPos())
            return -1;
        while (!m_eof && m_fd_pos < off) {
            fillBuffer();
        }
        if (m_eof && m_fd_pos < off)
            return -1;

        m_pos = off;
        return m_pos;
    }
    return seekRaw(off);
}

void SeekableInputStream::fillBuffer()
{
    if (m_buffer.size() > m_buffer.capacity() - 0x80000) {
        m_buffer.erase(std::begin(m_buffer), std::begin(m_buffer) + m_buffer.capacity() / 2);
    }
    size_t osize = m_buffer.size();
    m_buffer.resize(osize + 0x80000); // 512KiB
    ssize_t n = ::read(m_fd, m_buffer.data() + osize, m_buffer.size() - osize);
    if (n <= 0) {
        m_eof = true;
        m_buffer.resize(osize);
    } else {
        m_buffer.resize(osize + n);
        m_fd_pos += n;
    }
}

int64_t SeekableInputStream::seekRaw(int64_t pos)
{
    m_eof = false;
#ifdef _WIN32
    int64_t off = _lseeki64(m_fd, pos, SEEK_SET);
#else
    int64_t off = lseek(m_fd, pos, SEEK_SET);
#endif
    m_buffer.clear();
    if (off < 0) {
        m_eof = true;
        return -1;
    }
    m_pos = m_fd_pos = off;
    return off;
}
