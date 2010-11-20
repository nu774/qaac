#include <cstdlib>
#include <cstring>
#include "channel.h"
#include "util.h"
#include "win32util.h"

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
