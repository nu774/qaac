#include <stdarg.h>
#include <stdexcept>
#include "strutil.h"
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace strutil {
    template<> char *strsep(char **strp, const char *sep)
    {
        char *tok, *s;

        if (!strp || !(tok = *strp))
            return 0;
        if ((s = std::strpbrk(tok, sep))) {
            *s = 0;
            *strp = s + 1;
        } else
            *strp = 0;
        return tok;
    }
    template<> wchar_t *strsep(wchar_t **strp, const wchar_t *sep)
    {
        wchar_t *tok, *s;

        if (!strp || !(tok = *strp))
            return 0;
        if ((s = std::wcspbrk(tok, sep))) {
            *s = 0;
            *strp = s + 1;
        } else
            *strp = 0;
        return tok;
    }

#ifdef _WIN32
    std::string us2m(const std::string &src)
    {
        std::wstring wide = us2w(src);
        if (wide.empty()) return std::string();
        int nc = WideCharToMultiByte(CP_ACP, 0, wide.data(),
                                     static_cast<int>(wide.size()),
                                     0, 0, 0, 0);
        if (nc <= 0)
            throw std::runtime_error("us2m: conversion failed");
        std::string dst(nc, '\0');
        WideCharToMultiByte(CP_ACP, 0, wide.data(),
                            static_cast<int>(wide.size()),
                            &dst[0], nc, 0, 0);
        return dst;
    }
#endif

#if defined(_MSC_VER) || defined(__MINGW32__)
#ifndef vsnprintf
#define vsnprintf _vsnprintf
#endif
#endif
    std::string format(const char *fmt, ...)
    {
        va_list args;
        std::vector<char> buffer(128);

        va_start(args, fmt);
        int rc = vsnprintf(&buffer[0], buffer.size(), fmt, args);
        va_end(args);
        if (rc >= 0 && rc < static_cast<int>(buffer.size()))
            return std::string(&buffer[0], &buffer[rc]);
#if defined(_MSC_VER) || defined(__MINGW32__) 
        va_start(args, fmt);
        rc = _vscprintf(fmt, args);
        va_end(args);
        if (rc < 0) {
            // format failed
            return "";
        }
#endif
        buffer.resize(rc + 1);
        va_start(args, fmt);
        rc = vsnprintf(&buffer[0], buffer.size(), fmt, args);
        va_end(args);
        return std::string(&buffer[0], &buffer[rc]);
    }

#if defined(_MSC_VER) || defined(__MINGW32__) 
    std::wstring format(const wchar_t *fmt, ...)
    {
        va_list args;

        va_start(args, fmt);
        int rc = _vscwprintf(fmt, args);
        va_end(args);

        std::vector<wchar_t> buffer(rc + 1);

        va_start(args, fmt);
        rc = _vsnwprintf(&buffer[0], buffer.size(), fmt, args);
        va_end(args);

        return std::wstring(&buffer[0], &buffer[rc]);
    }
#endif

    /*
     * NUMBER ::= [0-9]+
     * TERM ::= NUMBER | NUMBER"-"NUMBER
     * RANGES ::= TERM | RANGES","TERM
     */
    bool parse_numeric_ranges(const char *s, std::vector<int> *nums,
                              int vmin, int vmax)
    {
        enum { NUMBER, TERM };
        char *end;
        std::vector<int> result;
        int n, state = NUMBER;

        do {
            n = strtoul(s, &end, 10);
            if (end == s || n < vmin || n > vmax)
                return false;
            if (state == NUMBER)
                result.push_back(n);
            else if (result.back() > n)
                return false;
            else {
                /* XXX: can consume HUGE memory depending on vmin and vmax */
                for (int k = result.back() + 1; k <= n; ++k)
                    result.push_back(k);
            }
            if (*end == ',')
                state = NUMBER;
            else if (*end == '-') {
                if (state == TERM)
                    return false;
                else
                    state = TERM;
            } else if (*end)
                return false;
        } while (*end && (s = end + 1));

        nums->swap(result);
        return true;
    }
}
