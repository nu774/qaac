#ifndef STRUTIL_HPP_INCLUDED
#define STRUTIL_HPP_INCLUDED

#include <cwchar>
#include <cwctype>
#include <cstring>
#include <cstdlib>
#include <string>
#include <vector>
#include <stdexcept>
#include <algorithm>
#include <iterator>
#include <utf8.h>

#if defined _MSC_VER
#ifndef strcasecmp
#define strcasecmp _stricmp
#endif
#endif

#if defined _MSC_VER
typedef intptr_t ssize_t;
#endif

namespace strutil {
    template<typename T> T *strsep(T **strp, const T *sep);
    template<> char *strsep(char **strp, const char *sep);
    template<> wchar_t *strsep(wchar_t **strp, const wchar_t *sep);

    template <typename T, typename Conv>
    inline
    std::basic_string<T> strtransform(const std::basic_string<T> &s, Conv conv)
    {
        std::basic_string<T> result;
        std::transform(s.begin(), s.end(), std::back_inserter(result), conv);
        return result;
    }
    inline
    std::string slower(const std::string &s)
    {
        return strtransform(s, tolower);
    }
    inline
    std::wstring wslower(const std::wstring &s)
    {
        return strtransform(s, towlower);
    }
    inline
    std::string supper(const std::string &s)
    {
        return strtransform(s, toupper);
    }
    inline
    std::wstring wsupper(const std::wstring &s)
    {
        return strtransform(s, towupper);
    }
    inline ssize_t strindex(const char *s, int ch)
    {
        const char *p = std::strchr(s, ch);
        return p ? p - s : -1;
    }
    inline ssize_t strindex(const wchar_t *s, int ch)
    {
        const wchar_t *p = std::wcschr(s, ch);
        return p ? p - s : -1;
    }
    template <typename T>
    void squeeze(T *str, const T *charset)
    {
        T *q = str;
        for (T *p = str; *p; ++p)
            if (strindex(charset, *p) == -1)
                *q++ = *p;
        *q = 0;
    }
    template <typename T>
    std::basic_string<T> squeeze(const std::basic_string<T> &str,
                                 const T *charset)
    {
        std::basic_string<T> result;
        std::copy_if(str.begin(), str.end(), std::back_inserter(result),
                     [&](T c) -> bool {
                        return strindex(charset, c) == -1;
                     });
        return result;
    }

    inline std::wstring us2w(const std::string &src)
    {
        std::wstring result;
#if WCHAR_MAX < 0x10000
        utf8::utf8to16(std::begin(src), std::end(src), std::back_inserter(result));
#else
        utf8::utf8to32(std::begin(src), std::end(src), std::back_inserter(result));
#endif
        return result;
    }

    inline std::string w2us(const std::wstring &src)
    {
        std::string result;
#if WCHAR_MAX < 0x10000
        utf8::utf16to8(std::begin(src), std::end(src), std::back_inserter(result));
#else
        utf8::utf32to8(std::begin(src), std::end(src), std::back_inserter(result));
#endif
        return result;
    }

#ifdef _WIN32
    std::string us2m(const std::string &src);
#endif

    std::string format(const char *fmt, ...);
    std::wstring format(const wchar_t *fmt, ...);

    template <typename T>
    std::basic_string<T> normalize_crlf(const T *s, const T *eol)
    {
        std::basic_string<T> result;
        T c;
        while ((c = *s++)) {
            if (c == '\r') {
                result.append(eol);
                if (*s == '\n')
                    ++s;
            }
            else if (c == '\n')
                result.append(eol);
            else
                result.push_back(c);
        }
        return result;
    }

    template<typename T>
    class Tokenizer {
        std::vector<T> m_buffer;
        const T *m_sep;
        T *m_tok;
    public:
        Tokenizer(const std::basic_string<T> &s, const T *sep)
            : m_sep(sep)
        {
            std::copy(s.begin(), s.end(), std::back_inserter(m_buffer));
            m_buffer.push_back(0);
            m_tok = &m_buffer[0];
        }
        T *next()
        {
            return strsep(&m_tok, m_sep);
        }
        T *rest()
        {
            return m_tok;
        }
    };

    bool parse_numeric_ranges(const char *s, std::vector<int> *nums,
                              int vmin=0, int vmax=99);

    inline const char *file_extension(const std::string &path)
    {
        size_t slash = path.find_last_of("/\\");
        size_t dot = path.find_last_of('.');
        if (dot == std::string::npos ||
            (slash != std::string::npos && dot < slash))
            return path.c_str() + path.size();
        return path.c_str() + dot;
    }
    inline const char *basename(const std::string &path)
    {
        size_t slash = path.find_last_of("/\\");
        return path.c_str() + (slash == std::string::npos ? 0 : slash + 1);
    }
}

#endif
