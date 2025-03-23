#include "Win32InputStream.h"
#include <io.h>
#include <sys/stat.h>

Win32InputStream::Win32InputStream(const std::wstring &path)
    : m_pos(0)
    , m_fd_pos(0)
    , m_eof(false)
    , m_seekable(false)
    , m_size(-1)
{
    if (path == L"-")
        m_fd = _fileno(stdin);
    else
        m_fd = _wsopen(path.c_str(), _O_RDONLY|_O_BINARY, _SH_DENYWR);
    if (m_fd == -1) {
        util::throw_crt_error(path);
    }
    struct _stat64 stb = { 0 };
    if (_fstat64(m_fd, &stb) == 0 && (stb.st_mode & _S_IFMT) == _S_IFREG) {
        m_size = stb.st_size;
        m_seekable = true;
    }
    m_buffer.reserve(0x800000); // 8MiB
}

Win32InputStream::~Win32InputStream()
{
    _close(m_fd);
}

int Win32InputStream::read(void *buf, unsigned size)
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

int64_t Win32InputStream::seek(int64_t off, int whence)
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
        return seek(off, SEEK_SET);
    }
    return seekRaw(off);
}

void Win32InputStream::fillBuffer()
{
    if (m_buffer.size() > m_buffer.capacity() - 0x80000) {
        m_buffer.clear();
    }
    size_t osize = m_buffer.size();
    m_buffer.resize(osize + 0x80000); // 512KiB
    int n = _read(m_fd, m_buffer.data() + osize, m_buffer.size() - osize);
    if (n <= 0) {
        m_eof = true;
        m_buffer.resize(osize);
    } else {
        m_buffer.resize(osize + n);
        m_fd_pos += n;
    }
}

int64_t Win32InputStream::seekRaw(int64_t pos)
{
    m_eof = false;
    int64_t off = _lseeki64(m_fd, pos, SEEK_SET);
    m_buffer.clear();
    if (off < 0) {
        m_eof = true;
        return -1;
    }
    m_pos = m_fd_pos = off;
    return off;
}
