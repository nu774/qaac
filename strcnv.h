#ifndef STRCNV_HPP_INCLUDED
#define STRCNV_HPP_INCLUDED

#include <cwchar>
#include <string>
#include <locale>
#include <stdexcept>

std::wstring &m2w(std::wstring &dst, const char *src, size_t srclen,
	const std::codecvt<wchar_t, char, std::mbstate_t> &cvt);

std::string &w2m(std::string &dst, const wchar_t *src, size_t srclen,
	       const std::codecvt<wchar_t, char, std::mbstate_t> &cvt);

inline
std::wstring m2w(const char *src, size_t srclen,
	const std::codecvt<wchar_t, char, std::mbstate_t> &cvt)
{
    std::wstring result;
    return m2w(result, src, srclen, cvt);
}
inline
std::wstring &m2w(std::wstring &dst, const char *src, size_t srclen,
	const std::locale &loc)
{
    typedef std::codecvt<wchar_t, char, std::mbstate_t> cvt_t;
    return m2w(dst, src, srclen, std::use_facet<cvt_t>(loc));
}
inline
std::wstring m2w(const char *src, size_t srclen, const std::locale &loc)
{
    std::wstring result;
    return m2w(result, src, srclen, loc);
}
inline
std::wstring &m2w(std::wstring &dst, const char *src, size_t srclen)
{
    return m2w(dst, src, srclen, std::locale(""));
}
inline
std::wstring m2w(const char *src, size_t srclen)
{
    return m2w(src, srclen, std::locale(""));
}
inline
std::wstring &m2w(std::wstring &dst, const std::string &src, 
	const std::codecvt<wchar_t, char, std::mbstate_t> &cvt)
{
    return m2w(dst, src.c_str(), src.size(), cvt);
}
inline
std::wstring m2w(const std::string &src, 
	const std::codecvt<wchar_t, char, std::mbstate_t> &cvt)
{
    return m2w(src.c_str(), src.size(), cvt);
}
inline
std::wstring &m2w(std::wstring &dst, const std::string &src,
	const std::locale& loc)
{
    return m2w(dst, src.c_str(), src.size(), loc);
}
inline
std::wstring m2w(const std::string &src, const std::locale& loc)
{
    return m2w(src.c_str(), src.size(), loc);
}
inline
std::wstring &m2w(std::wstring &dst, const std::string &src)
{
    return m2w(dst, src.c_str(), src.size());
}
inline
std::wstring m2w(const std::string &src)
{
    return m2w(src.c_str(), src.size());
}
inline
std::string w2m(const wchar_t *src, size_t srclen,
	       const std::codecvt<wchar_t, char, std::mbstate_t> &cvt)
{
    std::string result;
    return w2m(result, src, srclen, cvt);
}
inline
std::string &w2m(std::string &dst, const wchar_t *src, size_t srclen,
	const std::locale& loc)
{
    typedef std::codecvt<wchar_t, char, std::mbstate_t> cvt_t;
    return w2m(dst, src, srclen, std::use_facet<cvt_t>(loc));
}
inline
std::string w2m(const wchar_t *src, size_t srclen, const std::locale& loc)
{
    std::string result;
    return w2m(result, src, srclen, loc);
}
inline
std::string &w2m(std::string &dst, const wchar_t *src, size_t srclen)
{
    return w2m(dst, src, srclen, std::locale(""));
}
inline
std::string w2m(const wchar_t *src, size_t srclen)
{
    std::string result;
    return w2m(result, src, srclen);
}
inline
std::string &w2m(std::string &dst, const std::wstring &src,
	       const std::codecvt<wchar_t, char, std::mbstate_t> &cvt)
{
    return w2m(dst, src.c_str(), src.size(), cvt);
}
inline
std::string w2m(const std::wstring& src,
	       const std::codecvt<wchar_t, char, std::mbstate_t> &cvt)
{
    return w2m(src.c_str(), src.size(), cvt);
}
inline
std::string &w2m(std::string &dst, const std::wstring &src,
	const std::locale &loc)
{
    return w2m(dst, src.c_str(), src.size(), loc);
}
inline
std::string w2m(const std::wstring &src, const std::locale &loc)
{
    return w2m(src.c_str(), src.size(), loc);
}
inline
std::string &w2m(std::string &dst, const std::wstring &src)
{
    return w2m(dst, src.c_str(), src.size());
}
inline
std::string w2m(const std::wstring &src)
{
    return w2m(src.c_str(), src.size());
}

#endif
