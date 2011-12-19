#ifndef _WIN32UTIL_H
#define _WIN32UTIL_H

#include <cstdio>
#include <stdexcept>
#include <string>
#include <vector>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <shlwapi.h>
#include "util.h"

void throw_win32_error(const std::string& msg, DWORD error);

class ProcAddress {
    FARPROC fp_;
public:
    ProcAddress(HMODULE module, LPCSTR name) {
	fp_ = GetProcAddress(module, name);
    }
    template <typename T> operator T() const {
	return reinterpret_cast<T>(fp_);
    }
};

inline
std::wstring GetFullPathNameX(const std::wstring &path)
{
    DWORD length = GetFullPathNameW(path.c_str(), 0, 0, 0);
    std::vector<wchar_t> buffer(length);
    length = GetFullPathNameW(path.c_str(), buffer.size(), &buffer[0], 0);
    return std::wstring(&buffer[0], &buffer[length]);
}

inline
std::wstring PathReplaceExtension(const std::wstring &path, const wchar_t *ext)
{
    const wchar_t *beg = path.c_str();
    const wchar_t *end = PathFindExtensionW(beg);
    std::wstring s(beg, end);
    if (ext[0] != L'.') s.push_back(L'.');
    s += ext;
    return s;
}

inline
std::wstring PathFindFileNameX(const std::wstring &path)
{
    return PathFindFileNameW(path.c_str());
}

inline
std::wstring PathCombineX(const std::wstring &basedir,
	const std::wstring &filename)
{
    wchar_t buffer[MAX_PATH];
    PathCombineW(buffer, basedir.c_str(), filename.c_str());
    return buffer;
}

inline
std::wstring GetCurrentDirectoryX()
{
    DWORD len = GetCurrentDirectoryW(0, 0);
    std::vector<wchar_t> buffer(len + 1);
    len = GetCurrentDirectoryW(buffer.size(), &buffer[0]);
    return std::wstring(&buffer[0], &buffer[len]);
}

inline
std::wstring GetModuleFileNameX(HMODULE module)
{
    std::vector<wchar_t> buffer(32);
    DWORD cclen = GetModuleFileNameW(module, &buffer[0], buffer.size());
    while (cclen >= buffer.size() - 1) {
	buffer.resize(buffer.size() * 2);
	cclen = GetModuleFileNameW(module, &buffer[0], buffer.size());
    }
    return std::wstring(&buffer[0], &buffer[cclen]);
}

inline
std::wstring get_prefixed_fullpath(const wchar_t *path)
{
    std::wstring fullpath = GetFullPathNameX(path);
    if (fullpath.size() < 256)
	return fullpath;
    if (fullpath.size() > 2 && fullpath.substr(0, 2) == L"\\\\")
	fullpath.insert(2, L"?\\UNC\\");
    else
	fullpath.insert(0, L"\\\\?\\");
    return fullpath;
}

inline
FILE *wfopenx(const wchar_t *path, const wchar_t *mode)
{
    std::wstring fullpath = get_prefixed_fullpath(path);
    FILE *fp = _wfopen(fullpath.c_str(), mode);
    if (!fp)
	throw std::runtime_error(format("_wfopen: %ls: %s",
		    fullpath.c_str(), std::strerror(errno)));
    return fp;
}

FILE *win32_tmpfile(const wchar_t *prefix);

char *load_with_mmap(const wchar_t *path, uint64_t *size);

#ifdef __MINGW32__
#ifndef _STARTUP_INFO_DEFINED
typedef struct
{
        int newmode;
} _startupinfo;
#define _STARTUP_INFO_DEFINED
#endif  /* _STARTUP_INFO_DEFINED */
extern "C"
_CRTIMP int __wgetmainargs(int *, wchar_t ***, wchar_t ***,
	int, _startupinfo *);
#endif /* __MINGW32__ */
#endif
