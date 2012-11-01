#include <cstdlib>
#include <cstring>
#include "ioabst.h"
#include "util.h"
#include "strutil.h"
#ifdef _WIN32
#include "win32util.h"
#else
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#endif

StdioChannel::StdioChannel(const wchar_t *name)
{
    if (!std::wcscmp(name, L"-")) {
	m_name = L"<stdin>";
#ifdef _WIN32
	_setmode(0, _O_BINARY);
#endif
	m_fp = fileptr_t(stdin, no_close);
    } else {
	m_name = name;
	m_fp = fileptr_t(wfopenx(name, L"rb"), std::fclose);
    }
    test_seekable();
}

void StdioChannel::test_seekable()
{
#if defined(_MSC_VER) || defined(__MINGW32__)
    m_is_seekable = GetFileType(raw_handle()) == FILE_TYPE_DISK;
#else
    struct stat stb;
    fstat(fileno(m_fp.get()), &stb);
    m_is_seekable = !S_ISFIFO(stb.st_mode) && !S_ISSOCK(stb.st_mode);
#endif
}

ssize_t StdioChannel::read(void *buf, size_t count)
{
    return std::fread(buf, 1, count, m_fp.get());
}

int64_t StdioChannel::seek(int64_t offset, int whence)
{
#if defined(_MSC_VER) || defined(__MINGW32)
    CHECKCRT(_fseeki64(m_fp.get(), offset, whence));
#else
    CHECKCRT(fseeko(m_fp.get(), offset, whence));
#endif
    return tell();
}

int64_t StdioChannel::tell()
{
#ifdef _MSC_VER
    fpos_t off;
    CHECKCRT(std::fgetpos(m_fp.get(), &off));
    return off;
#else
    return ftello(m_fp.get());
#endif
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
	ssize_t n, nr;
	for (total = 0; total < count; total += n) {
	    n = static_cast<ssize_t>(std::min(count - total, bufsiz));
	    if ((nr = read(buf, n)) < n)
		break;
	}
	return total;
    }
}
