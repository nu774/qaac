#include <cstdlib>
#include <cstring>
#include "channel.h"
#include "util.h"
#include "win32util.h"

StdioChannel::StdioChannel(const wchar_t *name)
{
    if (!std::wcscmp(name, L"-")) {
	m_name = "<stdin>";
	_setmode(0, _O_BINARY);
	m_fp.swap(fileptr_t(stdin, no_close));
    } else {
	m_name = format("%ls", name);
	FILE *fp = _wfopen(name, L"rb");
	if (!fp) throw std::runtime_error(m_name + std::strerror(errno));
	m_fp.swap(fileptr_t(fp, fclose));
    }
    test_seekable();
}

void StdioChannel::test_seekable()
{
    m_is_seekable = GetFileType(raw_handle()) == FILE_TYPE_DISK;
}

ssize_t StdioChannel::read(void *buf, size_t count)
{
    return fread(buf, 1, count, m_fp.get());
}

int64_t StdioChannel::seek(int64_t offset, int whence)
{
    /*
     * XXX: By SPEC, we can only fsetpos() to where we have retrieved
     * pos_t via fgetpos().
     *
     * However, In VC++, fgetpos()/fsetpos() are nothing but 
     * _ftelli64()/_fseeki64() wrappers, and fpos_t is just an
     * 64 bit integer offset.
     * Therefore we can safely fsetpos() to anywhere, just like fseek().
     *
     * We can use _ftelli64()/_fseeki64() from VC8.0, but VC7.1 doesn't
     * export them, though it has them internally.
     * Therefore ugly #ifdef is needed..
     */
#if _MSC_VER < 1400
    fpos_t off = offset;
    if (whence == 1) 
	off += tell();
    else if (whence == 2) {
	DWORD high;
	DWORD low = GetFileSize(raw_handle(), &high);
	off += ((static_cast<int64_t>(high) << 32) | low);
    }
    if (std::fsetpos(m_fp.get(), &off))
	throw std::runtime_error(std::strerror(errno));
#else
    if (_fseeki64(m_fp.get(), offset, whence))
	throw std::runtime_error(std::strerror(errno));
#endif
    return tell();
}

int64_t StdioChannel::tell()
{
    fpos_t off;
    if (std::fgetpos(m_fp.get(), &off))
	throw std::runtime_error(std::strerror(errno));
    return off;
}

Win32Channel::Win32Channel(const wchar_t *name)
{
    if (!std::wcscmp(name, L"-")) {
	m_name = "<stdin>";
	m_fp.swap(fileptr_t(GetStdHandle(STD_INPUT_HANDLE), no_close));
    } else {
	m_name = format("%ls", name);
	HANDLE handle = CreateFileW(name, GENERIC_READ, FILE_SHARE_READ,
		0, OPEN_EXISTING, 0, 0);
	if (handle == INVALID_HANDLE_VALUE)
	    throw_win32_error(m_name, GetLastError());
	m_fp.swap(fileptr_t(handle, CloseHandle));
    }
    test_seekable();
}

void Win32Channel::test_seekable()
{
    m_is_seekable = GetFileType(m_fp.get()) == FILE_TYPE_DISK;
}

ssize_t Win32Channel::read(void *buf, size_t count)
{
    DWORD dwRead;
    return ReadFile(m_fp.get(), buf, count, &dwRead, 0) ? dwRead : -1;
}

int64_t Win32Channel::seek(int64_t offset, int whence)
{
    LARGE_INTEGER off, newoff;
    off.QuadPart = offset;
    return SetFilePointerEx(m_fp.get(), off, &newoff, whence)
	? newoff.QuadPart :  -1;
}

namespace __InputStreamImpl {

    size_t Seekable::read(void *buf, size_t count)
    {
	char *bufp = reinterpret_cast<char*>(buf),
	     *basep = bufp,
	     *endp = bufp + count;
	ssize_t rc;
	for (; bufp < endp; bufp += rc)
	    if ((rc = m_channel->read(bufp, endp - bufp)) <= 0)
		break;
	rc = bufp - basep;
	return rc;
    }
    size_t NonSeekable::read(void *buf, size_t count)
    {
	char *bufp = reinterpret_cast<char*>(buf),
	     *basep = bufp,
	     *endp = bufp + count;
	while (bufp < endp && m_pushback_buffer.size()) {
	    *bufp++ = m_pushback_buffer.back();
	    m_pushback_buffer.pop_back();
	}
	ssize_t rc;
	for (; bufp < endp; bufp += rc)
	    if ((rc = m_channel->read(bufp, endp - bufp)) <= 0)
		break;
	rc = bufp - basep;
	m_pos += rc;
	return rc;
    }
    int64_t NonSeekable::seek_forward(int64_t count)
    {
	int64_t seeked = 0;
	for (; seeked < count && m_pushback_buffer.size(); ++seeked, ++m_pos)
	    m_pushback_buffer.pop_back();
	if (seeked == count)
	    return seeked;
	return skip(count - seeked) + seeked;
    }
    int64_t NonSeekable::skip(int64_t count)
    {
	const int64_t bufsiz = 0x4000;
	char buf[bufsiz];
	int64_t total;
	ssize_t n;
	for (total = 0; total < count; total += n) {
	    n = static_cast<ssize_t>(std::min(count - total, bufsiz));
	    if ((n = read(buf, n)) < n)
		break;
	}
	return total;
    }
}
