#ifndef _CHANNEL_H
#define _CHANNEL_H
#include <cstddef>
#include <cstdio>
#include <fcntl.h>
#include <string>
#include <vector>
#include <stdint.h>
#include "shared_ptr.h"
#include "util.h"
#include "strcnv.h"
#if defined(_MSC_VER) || defined(__MINGW32__)
#include <io.h>
#include "win32util.h"
#endif

#if defined(_MSC_VER) || defined(__MINGW32__)
inline
FILE *wfopenx(const wchar_t *path, const wchar_t *mode)
{
    std::wstring fullpath = get_prefixed_fullpath(path);
    FILE *fp = _wfopen(fullpath.c_str(), mode);
    if (!fp)
        throw_crt_error(fullpath.c_str());
    return fp;
}
#else
inline
FILE *wfopenx(const wchar_t *path, const wchar_t *mode)
{
    std::string spath = w2m(path);
    std::string smode = nallow(mode);
    FILE *fp = std::fopen(spath.c_str(), smode.c_str());
    if (!fp)
        throw_crt_error(path);
    return fp;
}
#endif

struct IChannel {
    virtual ~IChannel() {}
    virtual IChannel *copy() = 0;
    virtual const wchar_t *name() = 0;
    virtual ssize_t read(void *buf, size_t count) = 0;
};

struct ISeekable: public IChannel {
    enum { kBegin, kCurrent, kEnd };
    virtual ~ISeekable() {}
    virtual bool seekable() = 0;
    virtual int64_t seek(int64_t offset, int whence) = 0;
    virtual int64_t tell() = 0;
};

typedef void *HANDLE;

/*
 * This class use stdio for buffered I/O, not for portability.
 * Unicode filename handling, 64bit seeking, and testing seekability
 * is not portable, and therefore is VC++ specific.
 */
class StdioChannel : public ISeekable {
    typedef x::shared_ptr<FILE> fileptr_t;
    fileptr_t m_fp;
    std::wstring m_name;
    bool m_is_seekable;
public:
    explicit StdioChannel(FILE *handle)
         : m_fp(handle, no_close),
           m_name(L"<stdin>")
    {
#ifdef _WIN32
        _setmode(0, _O_BINARY);
#endif
        test_seekable();
    }
    StdioChannel(const wchar_t *name);
    virtual StdioChannel *copy() { return new StdioChannel(*this); }
    const wchar_t *name() { return m_name.c_str(); }
    bool seekable() { return m_is_seekable; }
    ssize_t read(void *buf, size_t count);
    int64_t seek(int64_t offset, int whence);
    int64_t tell();
private:
#if defined(_MSC_VER) || defined(__MINGW32__)
    HANDLE raw_handle() {
        return reinterpret_cast<HANDLE>(_get_osfhandle(_fileno(m_fp.get())));
    }
#endif
    static void no_close(FILE *handle) {}
    void test_seekable();
};

template <class T>
class BinaryRead {
public:
    bool read16le(uint16_t *result)
    {
        if (((T*)(this))->read(result, 2) != 2)
            return false;
        *result = l2host16(*result);
        return true;
    }
    bool read16be(uint16_t *result)
    {
        if (((T*)(this))->read(result, 2) != 2)
            return false;
        *result = b2host16(*result);
        return true;
    }
    bool read32le(uint32_t *result)
    {
        if (((T*)(this))->read(result, 4) != 4)
            return false;
        *result = l2host32(*result);
        return true;
    }
    bool read32be(uint32_t *result)
    {
        if (((T*)(this))->read(result, 4) != 4)
            return false;
        *result = b2host32(*result);
        return true;
    }
};

class MemoryReader: public BinaryRead<MemoryReader>
{
    const uint8_t *m_position;
    const uint8_t *m_end;
public:
    MemoryReader(const void *beg, size_t size)
    {
        m_position = static_cast<const uint8_t*>(beg);
        m_end = m_position + size;
    }
    MemoryReader(const void *beg, const void *end)
    {
        m_position = static_cast<const uint8_t*>(beg);
        m_position = static_cast<const uint8_t*>(end);
    }
    ssize_t read(void *buffer, size_t count)
    {
        size_t n = std::min(m_end - m_position,
                            static_cast<ptrdiff_t>(count));
        std::memcpy(buffer, m_position, n);
        m_position += n;
        return n;
    }
    size_t skip(size_t count)
    {
        size_t n = std::min(m_end - m_position,
                            static_cast<ptrdiff_t>(count));
        m_position += n;
        return n;
    }
};

namespace __InputStreamImpl {

    struct Impl {
        virtual ~Impl() {}
        virtual const wchar_t *name() = 0;
        virtual size_t read(void *buf, size_t count) = 0;
        virtual int64_t seek(int64_t offset, int whence) = 0;
        virtual int64_t tell() = 0;
        virtual int64_t size() = 0;
        virtual void pushback(char ch) = 0;
        virtual void pushback(const char *s, size_t count) = 0;
        virtual int64_t seek_forward(int64_t count) = 0;
    };

    class Seekable: public Impl {
        typedef x::shared_ptr<ISeekable> channel_t;
        channel_t m_channel;
    public:
        Seekable(ISeekable &channel):
            m_channel(dynamic_cast<ISeekable*>(channel.copy())) {}
        const wchar_t *name() { return m_channel->name(); }
        size_t read(void *buf, size_t count);
        int64_t seek(int64_t offset, int whence)
        {
            return m_channel->seek(offset, whence);
        }
        int64_t tell() { return m_channel->tell(); }
        int64_t size()
        {
            int64_t curpos = tell();
            int64_t newpos = seek(0, ISeekable::kEnd);
            seek(curpos, ISeekable::kBegin);
            return newpos;
        }
        void pushback(char) { seek(-1, ISeekable::kCurrent); }
        void pushback(const char *, size_t count)
        {
            seek(-1 * static_cast<int64_t>(count), ISeekable::kCurrent);
        }
        int64_t seek_forward(int64_t count)
        {
            int64_t pos = tell();
            int64_t newpos = seek(count, ISeekable::kCurrent);
            return newpos < 0 ? 0 : newpos - pos;
        }
    };

    class NonSeekable: public Impl {
        typedef x::shared_ptr<IChannel> channel_t;
        channel_t m_channel;
        std::vector<char> m_pushback_buffer;
        uint64_t m_pos;
    public:
        NonSeekable(IChannel &channel): m_channel(channel.copy()), m_pos(0) {}
        const wchar_t *name() { return m_channel->name(); }
        size_t read(void *buf, size_t count);
        int64_t seek(int64_t, int) { return -1; }
        int64_t tell() { return m_pos; }
        int64_t size() { return -1; }
        void pushback(char ch) {
            m_pushback_buffer.push_back(ch);
            --m_pos;
        }
        void pushback(const char *s, size_t count)
        {
            for (ssize_t i = count - 1; i >= 0; --i)
                pushback(s[i]);
        }
        int64_t seek_forward(int64_t count);
    private:
        int64_t skip(int64_t count);
    };
}

class InputStream: public BinaryRead<InputStream> {
    typedef x::shared_ptr<__InputStreamImpl::Impl> impl_t;
    impl_t m_impl;
    bool m_seekable;
public:
    InputStream(IChannel &channel) 
    {
        ISeekable *pT = dynamic_cast<ISeekable*>(&channel);
        m_seekable = (pT && pT->seekable());
        __InputStreamImpl::Impl *ptr;
        if (m_seekable)
            ptr = new __InputStreamImpl::Seekable(*pT);
        else
            ptr = new __InputStreamImpl::NonSeekable(channel);
        impl_t foo(ptr);
        m_impl.swap(foo);
    }
    const wchar_t *name() { return m_impl->name(); }
    void pushback(char ch) { m_impl->pushback(ch); }
    void pushback(const char *s, size_t count) { m_impl->pushback(s, count); }
    size_t read(void *buf, size_t count) { return m_impl->read(buf, count); }
    bool seekable() { return m_seekable; }
    int64_t seek_forward(int64_t count)
    {
        return m_impl->seek_forward(count);
    }
    int64_t seek(int64_t offset, int whence)
    {
        return m_impl->seek(offset, whence);
    }
    int rewind()
    {
        return static_cast<int>(seek(0, ISeekable::kBegin));
    }
    int64_t tell() { return m_impl->tell(); }
    int64_t size() { return m_impl->size(); }
};
#endif
