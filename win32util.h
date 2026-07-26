#ifndef _WIN32UTIL_H
#define _WIN32UTIL_H

#include <cstdio>
#include <cstdarg>
#include <stdexcept>
#include <string>
#include <vector>
#include <memory>
#include <io.h>
#include <fcntl.h>
#include <share.h>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <shlwapi.h>
#pragma warning(push)
#pragma warning(disable: 4091)
#include <shlobj.h>
#pragma warning(pop)
#include "util.h"

#define HR(expr) (void)(win32::throwIfError((expr), #expr))

namespace win32 {
    class Timer {
        DWORD m_ticks;
    public:
        Timer() { m_ticks = GetTickCount(); };
        double ellapsed() {
            return (static_cast<double>(GetTickCount()) - m_ticks) / 1000.0;
        }
    };

    void throw_error(const std::string& msg, DWORD error);

    inline void throwIfError(HRESULT expr, const char *msg)
    {
        if (FAILED(expr)) throw_error(msg, expr);
    }

    inline std::wstring GetFullPathNameW_(const std::wstring &path)
    {
        DWORD length = GetFullPathNameW(path.c_str(), 0, 0, 0);
        std::vector<wchar_t> vec(length);
        length = GetFullPathNameW(path.c_str(), static_cast<DWORD>(vec.size()),
                                  &vec[0], 0);
        return std::wstring(&vec[0], &vec[length]);
    }

    inline std::string GetFullPathNameX(const std::string &path)
    {
        return strutil::w2us(GetFullPathNameW_(strutil::us2w(path)));
    }

    inline std::string PathReplaceExtension(const std::string &path,
                                            const char *ext)
    {
        std::wstring wpath = strutil::us2w(path);
        const wchar_t *beg = wpath.c_str();
        const wchar_t *end = PathFindExtensionW(beg);
        std::wstring s(beg, end);
        s += strutil::us2w(ext);
        return strutil::w2us(s);
    }

    // XXX: limited to MAX_PATH
    inline std::string PathCombineX(const std::string &basedir,
                                    const std::string &filename)
    {
        wchar_t buffer[MAX_PATH];
        PathCombineW(buffer, strutil::us2w(basedir).c_str(),
                     strutil::us2w(filename).c_str());
        return strutil::w2us(buffer);
    }

    inline std::wstring GetModuleFileNameX(HMODULE module)
    {
        std::vector<wchar_t> buffer(32);
        DWORD cclen = GetModuleFileNameW(module, &buffer[0],
                                         static_cast<DWORD>(buffer.size()));
        while (cclen >= buffer.size() - 1) {
            buffer.resize(buffer.size() * 2);
            cclen = GetModuleFileNameW(module, &buffer[0],
                                       static_cast<DWORD>(buffer.size()));
        }
        return std::wstring(&buffer[0], &buffer[cclen]);
    }

    inline bool MakeSureDirectoryPathExistsX(const std::string &path)
    {
        // SHCreateDirectoryEx() doesn't work with relative path
        std::wstring fullpath = GetFullPathNameW_(strutil::us2w(path));
        std::vector<wchar_t> buf(fullpath.begin(), fullpath.end());
        buf.push_back(0);
        wchar_t *pos = PathFindFileNameW(buf.data());
        *pos = 0;
        int rc = SHCreateDirectoryExW(nullptr, buf.data(), nullptr);
        return rc == ERROR_SUCCESS;
    }

    inline std::string get_module_directory(HMODULE module=0)
    {
        std::wstring path = GetModuleFileNameX(module);
        const wchar_t *fpos = PathFindFileNameW(path.c_str());
        return strutil::w2us(path.substr(0, fpos - path.c_str()));
    }

    inline std::wstring prefixed_path(const wchar_t *path)
    {
        std::wstring fullpath = GetFullPathNameW_(path);
        if (fullpath.size() < 256)
            return fullpath;
        if (fullpath.size() > 2 && fullpath.substr(0, 2) == L"\\\\")
            fullpath.insert(2, L"?\\UNC\\");
        else
            fullpath.insert(0, L"\\\\?\\");
        return fullpath;
    }

    inline FILE *wfopenx(const std::string &path, const char *mode)
    {
        std::wstring fullpath = win32::prefixed_path(strutil::us2w(path).c_str());
        std::wstring wmode = strutil::us2w(mode);
        int share = _SH_DENYRW;
        if (std::wcschr(wmode.c_str(), L'r') && !std::wcschr(wmode.c_str(), L'+'))
            share = _SH_DENYWR;
        FILE *fp = _wfsopen(fullpath.c_str(), wmode.c_str(), share);
        if (!fp) {
            if (_doserrno) throw_error(path, _doserrno);
            util::throw_crt_error(path);
        }
        return fp;
    }
    inline std::shared_ptr<FILE> fopen(const std::string &path,
                                       const char *mode)
    {
        auto noop_close = [](FILE *){};
        if (path != "-")
            return std::shared_ptr<FILE>(wfopenx(path, mode),
                                         std::fclose);
        else if (std::strchr(mode, 'r'))
            return std::shared_ptr<FILE>(stdin, noop_close);
        else
            return std::shared_ptr<FILE>(stdout, noop_close);
    }

    inline HANDLE get_handle(int fd)
    {
        return reinterpret_cast<HANDLE>(_get_osfhandle(fd));
    }
    inline bool is_seekable(HANDLE fh)
    {
        return GetFileType(fh) == FILE_TYPE_DISK;
    }
    inline bool is_seekable(int fd)
    {
        return is_seekable(get_handle(fd));
    }

    inline void write_utf8(FILE *fp, const std::string &utf8text)
    {
        HANDLE h = get_handle(_fileno(fp));
        DWORD mode;
        if (GetConsoleMode(h, &mode)) {
            std::wstring wtext = strutil::us2w(utf8text);
            DWORD written;
            WriteConsoleW(h, wtext.c_str(),
                         static_cast<DWORD>(wtext.size()), &written, 0);
        } else {
            std::fwrite(utf8text.data(), 1, utf8text.size(), fp);
        }
    }

    inline int vfprintf(FILE *fp, const char *fmt, va_list args)
    {
        va_list args2;
        va_copy(args2, args);
        int rc = _vscprintf(fmt, args);
        std::vector<char> buffer(rc + 1);
        vsnprintf(buffer.data(), buffer.size(), fmt, args2);
        va_end(args2);
        write_utf8(fp, buffer.data());
        return rc;
    }
    inline int fprintf(FILE *fp, const char *fmt, ...)
    {
        va_list ap;
        va_start(ap, fmt);
        // qualified: an unqualified call here is ambiguous between
        // win32::vfprintf and ::vfprintf (ADL considers the global
        // namespace too, since FILE is declared there).
        int rc = win32::vfprintf(fp, fmt, ap);
        va_end(ap);
        return rc;
    }

    FILE *tmpfile(const std::string &prefix);

    char *load_with_mmap(const std::string &path, uint64_t *size);

    int create_named_pipe(const std::string &path);

    std::string get_dll_version_for_locale(HMODULE hDll, WORD langid);

    bool is_same_file(HANDLE ha, HANDLE hb);

    inline bool is_same_file(int fda, int fdb)
    {
        return is_same_file(get_handle(fda), get_handle(fdb));
    }
}
#endif
