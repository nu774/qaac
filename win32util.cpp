#include "win32util.h"
#include "util.h"
#include <io.h>
#include <fcntl.h>

void throw_win32_error(const std::string &msg, DWORD code)
{
    LPSTR pszMsg;
    FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM,
		   0,
		   code,
		   MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		   (LPSTR)&pszMsg,
		   0,
		   0);
    squeeze(pszMsg, "\r\n");
    std::runtime_error e(format("%s: %s", msg.c_str(), pszMsg));
    LocalFree(pszMsg);
    throw e;
}

FILE *win32_tmpfile(const wchar_t *prefix)
{
    wchar_t *tmpname = _wtempnam(GetTempPathX().c_str(), prefix);
    HANDLE fh = CreateFileW(get_prefixed_fullpath(tmpname).c_str(),
	    GENERIC_READ|GENERIC_WRITE,
	    0, 0, CREATE_ALWAYS,
	    FILE_ATTRIBUTE_TEMPORARY|FILE_FLAG_DELETE_ON_CLOSE, 0);
    if (fh == INVALID_HANDLE_VALUE)
	throw_win32_error(format("%ls", tmpname).c_str(), GetLastError());
    int fd = _open_osfhandle(reinterpret_cast<intptr_t>(fh),
	    _O_BINARY|_O_RDWR);
    if (fd == -1) {
	CloseHandle(fh);
	throw std::runtime_error(std::strerror(errno));
    }
    FILE *fp = _fdopen(fd, "w+");
    if (!fp) {
	_close(fd);
	throw std::runtime_error(std::strerror(errno));
    }
    return fp;
}
