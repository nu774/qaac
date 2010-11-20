#ifndef _CHANNEL_H
#define _CHANNEL_H
#include <cstdio>
#include <string>
#include <vector>
#include <stdint.h>
#if _MSC_VER >= 1500
# include <memory>
#else
# include <boost/tr1/memory.hpp>
#endif
#ifdef _MSC_VER
#include <intsafe.h>
  typedef SSIZE_T ssize_t;
#endif

struct IChannel {
    virtual ~IChannel() {}
    virtual IChannel *copy() = 0;
    virtual const char *name() = 0;
    virtual ssize_t read(void *buf, size_t count) = 0;
};

struct ISeekable: public IChannel {
    enum { kBegin, kCurrent, kEnd };
    virtual ~ISeekable() {}
    virtual bool seekable() = 0;
    virtual int64_t seek(int64_t offset, int whence) = 0;
};

class Win32Channel : public ISeekable {
    typedef std::tr1::shared_ptr<void> fileptr_t;
    fileptr_t m_fp;
    std::string m_name;
    bool m_is_seekable;
public:
    explicit Win32Channel(void *handle)
	: m_name("<stdin>"),
	  m_fp(handle, no_close)
    {
	test_seekable();
    }
    Win32Channel(const wchar_t *name);
    virtual Win32Channel *copy() { return new Win32Channel(*this); }
    const char *name() { return m_name.c_str(); }
    bool seekable() { return m_is_seekable; }
    ssize_t read(void *buf, size_t count);
    int64_t seek(int64_t offset, int whence);
private:
    static void no_close(void *handle) {}
    void test_seekable();
};

template <class T>
class BinaryRead {
public:
    bool read16le(uint16_t *result)
    {
	if (!readn(result, 2)) return false;
	*result = l2host16(*result);
	return true;
    }
    bool read16be(uint16_t *result)
    {
	if (!readn(result, 2)) return false;
	*result = b2host16(*result);
	return true;
    }
    bool read32le(uint32_t *result)
    {
	if (!readn(result, 4)) return false;
	*result = l2host32(*result);
	return true;
    }
    bool read32be(uint32_t *result)
    {
	if (!readn(result, 4)) return false;
	*result = b2host32(*result);
	return true;
    }
private:
    bool readn(void *buff, size_t n)
    {
	T *pT = static_cast<T*>(this);
	return pT->read(buff, n) == n;
    }
};

namespace __InputStreamImpl {

    struct Impl {
	virtual ~Impl() {}
	virtual size_t read(void *buf, size_t count) = 0;
	virtual int64_t seek(int64_t offset, int whence) = 0;
	virtual int64_t tell() = 0;
	virtual int64_t size() = 0;
	virtual void pushback(char ch) = 0;
	virtual void pushback(const char *s, size_t count) = 0;
	virtual int64_t seek_forward(int64_t count) = 0;
    };

    class Seekable: public Impl {
	typedef std::tr1::shared_ptr<ISeekable> channel_t;
	channel_t m_channel;
    public:
	Seekable(ISeekable &channel):
	    m_channel(dynamic_cast<ISeekable*>(channel.copy())) {}
	size_t read(void *buf, size_t count);
	int64_t seek(int64_t offset, int whence)
	{
	    return m_channel->seek(offset, whence);
	}
	int64_t tell() { return seek(0, ISeekable::kCurrent); }
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
	typedef std::tr1::shared_ptr<IChannel> channel_t;
	channel_t m_channel;
	std::vector<char> m_pushback_buffer;
	uint64_t m_pos;
    public:
	NonSeekable(IChannel &channel): m_channel(channel.copy()), m_pos(0) {}
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
	    for (int i = count - 1; i >= 0; --i)
		pushback(s[i]);
	}
	int64_t seek_forward(int64_t count);
    private:
	int64_t skip(int64_t count);
    };
}

class InputStream: public BinaryRead<InputStream> {
    typedef std::tr1::shared_ptr<__InputStreamImpl::Impl> impl_t;
    impl_t m_impl;
    bool m_seekable;
public:
    InputStream(IChannel &channel) 
    {
	ISeekable *pT = dynamic_cast<ISeekable*>(&channel);
	m_seekable = (pT && pT->seekable());
	if (m_seekable)
	    m_impl.swap(impl_t(new __InputStreamImpl::Seekable(*pT)));
	else
	    m_impl.swap(impl_t(new __InputStreamImpl::NonSeekable(channel)));
    }
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
