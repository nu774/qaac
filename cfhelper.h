#ifndef _CFHELPER_H
#define _CFHELPER_H

#include <cstdlib>
#include <iostream>
#include <vector>
#include "shared_ptr.h"
#include <stdexcept>
#include <MacErrors.h>
#include <MacMemory.h>
#include <CoreFoundation.h>
#include "util.h"

inline void TryE(int err, const char *msg)
{
    if (err)
	throw std::runtime_error(format("Error: %d: %s", err, msg));
}

inline void TryF(int err, const char *msg, const std::wstring &filename)
{
    std::string s = "";
    const char *syserr = 0;
    if (err == couldNotResolveDataRef)
	syserr = "No such file or directory";
    else if (err == ioErr)
	syserr = "I/O error";
    else if (err == pathTooLongErr)
	syserr = "Filename too long";
    else if (err == noMovieFound)
	syserr = "Unknown input format";

    if (syserr)
	s = format("Error: %ls: %s: %s", filename.c_str(), syserr, msg);
    else
	s = format("Error: %ls: %d: %s", filename.c_str(), err, msg);
    if (err) throw std::runtime_error(s);
}
#define TRYE(expr) (void)(TryE((expr), #expr))

#define TRYF(expr, file) (void)(TryF((expr), #expr, file))

template <typename T>
inline void Ensure_CFType(CFTypeRef v) {}

template <>
inline void Ensure_CFType<CFNumberRef>(CFTypeRef v)
{
    if (CFGetTypeID(v) != CFNumberGetTypeID())
	throw std::runtime_error("CFNumber is expected");
}

template <>
inline void Ensure_CFType<CFStringRef>(CFTypeRef v)
{
    if (CFGetTypeID(v) != CFStringGetTypeID())
	throw std::runtime_error("CFString is expected");
}

template <>
inline void Ensure_CFType<CFArrayRef>(CFTypeRef v)
{
    if (CFGetTypeID(v) != CFArrayGetTypeID())
	throw std::runtime_error("CFArray is expected");
}

template <>
inline void Ensure_CFType<CFDictionaryRef>(CFTypeRef v)
{
    if (CFGetTypeID(v) != CFArrayGetTypeID())
	throw std::runtime_error("CFDictionary is expected");
}

template <typename T>
inline T DynamicCast_CFType(CFTypeRef v)
{
    Ensure_CFType<T>(v);
    return reinterpret_cast<T>(v);
}

class CFStringX {
    x::shared_ptr<const __CFString> m_instance;
public:
    CFStringX() {}
    CFStringX(CFStringRef s) : m_instance(s, CFRelease) {}
    operator CFStringRef() const { return m_instance.get(); }
    operator CFTypeRef() const { return m_instance.get(); }
};

inline std::wstring CF2W(CFStringRef str)
{
    CFIndex length = CFStringGetLength(str);
    if (!length) return L"";
    std::vector<UniChar> buffer(length);
    CFRange range = { 0, length };
    CFStringGetCharacters(str, range, &buffer[0]);
    return std::wstring(buffer.begin(), buffer.end());
}

inline CFStringX W2CF(const std::wstring &str)
{
    const UniChar *ss = reinterpret_cast<const UniChar *>(str.c_str());
    return CFStringX(CFStringCreateWithCharacters(0, ss, str.size()));
}

class CFNumberX {
    x::shared_ptr<const __CFNumber> m_instance;
public:
    CFNumberX(CFNumberRef n) : m_instance(n, CFRelease) {}
    operator CFNumberRef() const { return m_instance.get(); }
    operator CFTypeRef() const { return m_instance.get(); }
};

template <typename T> CFNumberX CFNumberCreateT(T n);
template <>
inline CFNumberX CFNumberCreateT<int32_t>(int32_t n)
{
    return CFNumberX(CFNumberCreate(0, kCFNumberSInt32Type, &n));
}

template <typename T>
inline T CFDictionaryGetValueT(CFDictionaryRef dict, CFStringRef key)
{
    CFTypeRef ref = CFDictionaryGetValue(dict, key);
    if (!ref)
	throw std::runtime_error(format("CFDictionaryRef: Key [%ls] not found",
		    CF2W(key).c_str()));
    return DynamicCast_CFType<T>(ref);
}

class HandleX {
    x::shared_ptr<Ptr> m_instance;
public:
    HandleX() {}
    HandleX(Handle n) : m_instance(n, DisposeHandle) {}
    operator Handle() const { return m_instance.get(); }
};

inline void DisposeHandleX(void *handle)
{
    DisposeHandle(reinterpret_cast<Handle>(handle));
}

CFDictionaryRef
SearchCFDictArray(CFArrayRef ref, CFStringRef key, CFStringRef value);

CFTypeRef CloneCFObject(CFTypeRef value);

template <typename T> class CFArrayT {
    x::shared_ptr<const __CFArray> m_self;
    size_t m_size;
public:
    typedef T value_type;

    CFArrayT(): m_size(0) {}
    CFArrayT(CFArrayRef ref)
	: m_self(ref, CFRelease),
	  m_size(CFArrayGetCount(ref))
    {}
    void swap(CFArrayT<T> &other)
    {
	std::swap(m_size, other.m_size);
	m_self.swap(other.m_self);
    }
    operator CFArrayRef() const { return m_self.get(); }
    operator CFTypeRef() const { return m_self.get(); }
    size_t size() const { return m_size; }
    T operator[](size_t index) const { return at(index); }
    T at(size_t index) const
    {
	return DynamicCast_CFType<T>(
		CFArrayGetValueAtIndex(m_self.get(), index));
    }
};

#endif
