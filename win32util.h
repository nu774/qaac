#ifndef _WIN32UTIL_H
#define _WIN32UTIL_H

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
    std::wstring s(path.c_str(), PathFindExtensionW(path.c_str()));
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
std::wstring GetModuleFileNameX()
{
    std::vector<wchar_t> buffer(32);
    DWORD cclen = GetModuleFileNameW(0, &buffer[0], buffer.size());
    while (cclen >= buffer.size() - 1) {
	buffer.resize(buffer.size() * 2);
	cclen = GetModuleFileNameW(0, &buffer[0], buffer.size());
    }
    return std::wstring(&buffer[0], &buffer[cclen]);
}

class DirectorySaver {
    std::wstring m_pwd;
public:
    DirectorySaver() { m_pwd = GetCurrentDirectoryX(); }
    ~DirectorySaver() { SetCurrentDirectoryW(m_pwd.c_str()); }
};

#endif
