#include "win32util.h"
#include "util.h"
#include <io.h>
#include <fcntl.h>
#include "strutil.h"

namespace win32 {
    void throw_error(const std::wstring &msg, DWORD code)
    {
        LPWSTR pszMsg = 0;
        FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                       FORMAT_MESSAGE_FROM_SYSTEM,
                       0,
                       code,
                       MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                       (LPWSTR)&pszMsg,
                       0,
                       0);
        std::wstring ss;
        if (pszMsg) {
            strutil::squeeze(pszMsg, L"\r\n");
            ss = strutil::format(L"%s: %s", msg.c_str(), pszMsg);
            LocalFree(pszMsg);
        }
        else if (code < 0xfe00)
            ss = strutil::format(L"%d: %s", code, msg.c_str());
        else
            ss = strutil::format(L"%08x: %s", code, msg.c_str());
        throw std::runtime_error(strutil::w2us(ss));
    }

    FILE *tmpfile(const wchar_t *prefix)
    {
        std::wstring sprefix =
            strutil::format(L"%s.%d.", prefix, GetCurrentProcessId());
        wchar_t *tmpname = _wtempnam(0, sprefix.c_str());
        std::shared_ptr<wchar_t> tmpname_p(tmpname, std::free);
        HANDLE fh = CreateFileW(prefixed_path(tmpname).c_str(),
                                GENERIC_READ | GENERIC_WRITE,
                                0, 0, CREATE_ALWAYS,
                                FILE_ATTRIBUTE_TEMPORARY |
                                FILE_FLAG_DELETE_ON_CLOSE,
                                0);
        if (fh == INVALID_HANDLE_VALUE)
            throw_error(tmpname, GetLastError());
        int fd = _open_osfhandle(reinterpret_cast<intptr_t>(fh),
                _O_BINARY|_O_RDWR);
        if (fd == -1) {
            CloseHandle(fh);
            util::throw_crt_error("win32::tmpfile: open_osfhandle()");
        }
        FILE *fp = _fdopen(fd, "w+");
        if (!fp) {
            _close(fd);
            util::throw_crt_error("win32::tmpfile: _fdopen()");
        }
        return fp;
    }

    char *load_with_mmap(const wchar_t *path, uint64_t *size)
    {
        std::wstring fullpath = prefixed_path(path);
        HANDLE hFile = CreateFileW(fullpath.c_str(), GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
        if (hFile == INVALID_HANDLE_VALUE)
            throw_error(fullpath, GetLastError());
        DWORD sizeHi, sizeLo;
        sizeLo = GetFileSize(hFile, &sizeHi);
        *size = (static_cast<uint64_t>(sizeHi) << 32) | sizeLo;
        HANDLE hMap = CreateFileMappingW(hFile, 0, PAGE_READONLY, 0, 0, 0);
        DWORD err = GetLastError();
        CloseHandle(hFile);
        if (!hMap) throw_error("CreateFileMapping", err);
        char *view =
            reinterpret_cast<char*>( MapViewOfFile(hMap, FILE_MAP_READ,
                                                   0, 0, 0));
        CloseHandle(hMap);
        return view;
    }

    int create_named_pipe(const wchar_t *path)
    {
        HANDLE fh = CreateNamedPipeW(path,
                                     PIPE_ACCESS_OUTBOUND,
                                     PIPE_TYPE_BYTE | PIPE_WAIT,
                                     1, 0, 0, 0, 0);
        if (fh == INVALID_HANDLE_VALUE)
            throw_error(path, GetLastError());
        ConnectNamedPipe(fh, 0);
        return _open_osfhandle(reinterpret_cast<intptr_t>(fh),
                               _O_WRONLY | _O_BINARY);
    }

    std::string get_dll_version_for_locale(HMODULE hDll, WORD langid)
    {
        HRSRC hRes = FindResourceExW(hDll,
                                     RT_VERSION,
                                     MAKEINTRESOURCEW(VS_VERSION_INFO),
                                     langid);
        if (!hRes)
            win32::throw_error("FindResourceExW", GetLastError());
        std::string data;
        {
            DWORD cbres = SizeofResource(hDll, hRes);
            HGLOBAL hMem = LoadResource(hDll, hRes);
            if (hMem) {
                char *pc = static_cast<char*>(LockResource(hMem));
                if (cbres && pc)
                    data.assign(pc, cbres);
                FreeResource(hMem);
            }
        }
        // find dwSignature of VS_FIXEDFILEINFO
        size_t pos = data.find("\xbd\x04\xef\xfe");
        if (pos != std::string::npos) {
            VS_FIXEDFILEINFO vfi;
            std::memcpy(&vfi, data.c_str() + pos, sizeof vfi);
            WORD v[4];
            v[0] = HIWORD(vfi.dwFileVersionMS);
            v[1] = LOWORD(vfi.dwFileVersionMS);
            v[2] = HIWORD(vfi.dwFileVersionLS);
            v[3] = LOWORD(vfi.dwFileVersionLS);
            return strutil::format("%u.%u.%u.%u", v[0],v[1],v[2],v[3]);
        }
        return "";
    }
}
