#include "win32util.h"
#include "util.h"
#include <io.h>
#include <fcntl.h>
#include "strcnv.h"

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
    std::string ss;
    if (pszMsg) {
	squeeze(pszMsg, "\r\n");
	ss = format("%s: %s", msg.c_str(), pszMsg);
	LocalFree(pszMsg);
    }
    else if (code < 0xfe00)
	ss = format("ERROR %d: %s", code, msg.c_str());
    else
	ss = format("ERROR %08x: %s", code, msg.c_str());
    throw std::runtime_error(ss);
}

FILE *win32_tmpfile(const wchar_t *prefix)
{
    std::wstring sprefix = format(L"%s.%d.", prefix, GetCurrentProcessId());
    wchar_t *tmpname = _wtempnam(0, sprefix.c_str());
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

char *load_with_mmap(const wchar_t *path, uint64_t *size)
{
    std::wstring fullpath = get_prefixed_fullpath(path);
    HANDLE hFile = CreateFileW(fullpath.c_str(), GENERIC_READ,
	    0, 0, OPEN_EXISTING, 0, 0);
    if (hFile == INVALID_HANDLE_VALUE)
	throw_win32_error(w2m(fullpath), GetLastError());
    DWORD sizeHi, sizeLo;
    sizeLo = GetFileSize(hFile, &sizeHi);
    *size = (static_cast<uint64_t>(sizeHi) << 32) | sizeLo;
    HANDLE hMap = CreateFileMappingW(hFile, 0, PAGE_READONLY, 0, 0, 0);
    DWORD err = GetLastError();
    CloseHandle(hFile);
    if (hMap <= 0) throw_win32_error("CreateFileMapping", err);
    char *view = reinterpret_cast<char*>(
	    MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0));
    CloseHandle(hMap);
    return view;
}
