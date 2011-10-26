#ifndef _QTHELPER_H
#define _QTHELPER_H

#include <cstdio>
#include <cassert>
#include <QTML.h>
#include <Movies.h>
#include "cfhelper.h"
#include "chanmap.h"
#include "win32util.h"

struct QTInitializer {
    QTInitializer() {
	DirectorySaver __saver__;
	InitializeQTML(0);
	EnterMovies();
    }
    ~QTInitializer() {
	ExitMovies();
	TerminateQTML();
    }
};

inline ByteCount AudioChannelLayout_length(const AudioChannelLayout *acl)
{
    ByteCount n = offsetof(AudioChannelLayout, mChannelDescriptions);
    n += acl->mNumberChannelDescriptions * sizeof(AudioChannelDescription);
    return n;
}

class AudioChannelLayoutX {
public:
    typedef x::shared_ptr<AudioChannelLayout> owner_t;

    AudioChannelLayoutX() { create(0); }
    explicit AudioChannelLayoutX(size_t channel_count)
    {
	create(channel_count);
    }
    AudioChannelLayoutX(const AudioChannelLayout &layout)
    {
	attach(&layout);
    }
    AudioChannelLayoutX(const AudioChannelLayout *layout)
    {
	attach(layout);
    }
    operator AudioChannelLayout *() { return m_instance.get(); }
    operator const AudioChannelLayout *() const { return m_instance.get(); }
    AudioChannelLayout *operator->() { return m_instance.get(); }
    ByteCount size() { return AudioChannelLayout_length(m_instance.get()); }
    unsigned numChannels() const
    {
	switch (m_instance->mChannelLayoutTag) {
	case kAudioChannelLayoutTag_UseChannelDescriptions:
	    return m_instance->mNumberChannelDescriptions;
	case kAudioChannelLayoutTag_UseChannelBitmap:
	    return bitcount(m_instance->mChannelBitmap);
	}
	return AudioChannelLayoutTag_GetNumberOfChannels(
		m_instance->mChannelLayoutTag);
    }
    static AudioChannelLayoutX CreateDefault(unsigned nchannels)
    {
	return FromBitmap(GetDefaultChannelMask(nchannels));
    }
    static AudioChannelLayoutX FromChannelMap(const std::vector<uint32_t> &map)
    {
	return FromBitmap(GetChannelMask(map));
    }
    static AudioChannelLayoutX FromBitmap(uint32_t bitmap)
    {
	size_t nc = bitcount(bitmap);
	AudioChannelLayoutX layout(nc);
	layout->mChannelLayoutTag = GetLayoutTag(bitmap);
	if (layout->mChannelLayoutTag
		== kAudioChannelLayoutTag_UseChannelBitmap)
	    layout->mChannelBitmap = bitmap;
	MapChannelLabel(&layout->mChannelDescriptions[0], bitmap);
	return layout;
    }
private:
    void create(size_t channel_count)
    {
	size_t size = offsetof(AudioChannelLayout, mChannelDescriptions);
	size += channel_count * sizeof(AudioChannelDescription);
	m_instance = owner_t(
		reinterpret_cast<AudioChannelLayout*>(xcalloc(1, size)),
		std::free);
	m_instance->mNumberChannelDescriptions = channel_count;
    }
    void attach(const AudioChannelLayout *layout)
    {
	ByteCount size = AudioChannelLayout_length(layout);
	owner_t p = owner_t(
	    reinterpret_cast<AudioChannelLayout*>(xmalloc(size)),
	    std::free);
	std::memcpy(p.get(), layout, size);
	m_instance.swap(p);
    }
    owner_t m_instance;
};

template <class T>
class PropertySupport {
    void _check(int err, const char *msg, OSType klass, OSType id)
    {
	if (err) {
	    std::string s = format("Error: %d: %s: %s/%s", err, msg,
		    fourcc(klass).svalue, fourcc(id).svalue);
	    throw std::runtime_error(s);
	}
    }
public:
    void getPropertyInfo(
	    OSType inPropClass,
	    OSType inPropID,
	    OSType *outPropType = 0,
	    ByteCount *outPropValueSize = 0,
	    UInt32 *outPropertyFlags = 0)
    {
	T *pT = static_cast<T*>(this);
	_check(pT->_getPropertyInfo(
			inPropClass,
			inPropID,
			outPropType,
			outPropValueSize,
			outPropertyFlags),
		"getPropertyInfo", inPropClass, inPropID);
    }
    void getProperty(
	    OSType inPropClass,
	    OSType inPropID,
	    ByteCount inPropValueSize,
	    void *outPropValueAddress,
	    ByteCount *outPropValueSizeUsed = 0)
    {
	T *pT = static_cast<T*>(this);
	_check(pT->_getProperty(
			inPropClass,
			inPropID,
			inPropValueSize,
			outPropValueAddress,
			outPropValueSizeUsed),
		"getProperty", inPropClass, inPropID);
    }
    void setProperty(
	    OSType inPropClass,
	    OSType inPropID,
	    ByteCount inPropValueSize,
	    const void * inPropValueAddress)
    {
	T *pT = static_cast<T*>(this);
	_check(pT->_setProperty(
			inPropClass,
			inPropID,
			inPropValueSize,
			inPropValueAddress),
		"setProperty", inPropClass, inPropID);
    }
    template <typename U>
    U getPodProperty(OSType cls, OSType id)
    {
	U data;
	getProperty(cls, id, sizeof(U), &data);
	return data;
    }
    template <typename U>
    void getPodProperty(OSType cls, OSType id, U *data)
    {
	getProperty(cls, id, sizeof(U), data);
    }
    template <typename U>
    void setPodProperty(OSType cls, OSType id, const U &value)
    {
	setProperty(cls, id, sizeof(U), &value);
    }
    template <typename U>
    void getVectorProperty(OSType cls, OSType id, std::vector<U> *result)
    {
	ByteCount size;
	getPropertyInfo(cls, id, 0, &size);
	std::vector<U> buffer(size / sizeof(U));
	if (size) getProperty(cls, id, size, &buffer[0]);
	result->swap(buffer);
    }
    /* for variable-sized struct */
    template <typename U>
    void getPointerProperty(
	    OSType cls,
	    OSType id,
	    x::shared_ptr<U> *result,
	    ByteCount *size = 0)
    {
	ByteCount sz;
	getPropertyInfo(cls, id, 0, &sz);
	x::shared_ptr<U> ptr(
		reinterpret_cast<U*>(xmalloc(sz)), std::free);
	getProperty(cls, id, sz, ptr.get());
	if (size) *size = sz;
	result->swap(ptr);
    }
};

class ComponentX: public PropertySupport<ComponentX> {
    typedef x::shared_ptr<ComponentInstanceRecord> owner_t;
    owner_t m_instance;
protected:
    void attach(ComponentInstance instance)
    {
	m_instance = owner_t(instance, CloseComponent);
    }
public:
    ComponentX() {}
    ComponentX(ComponentInstance instance)
	: m_instance(instance, CloseComponent) {}
    virtual ~ComponentX() {}
    static ComponentX OpenDefault(
	    OSType componentType, OSType componentSubType)
    {
	ComponentInstance ci;
	OSErr err = ::OpenADefaultComponent(
		componentType, componentSubType, &ci);
	if (err) {
	    std::string s = format("Error: %d: OpenADefaultComponent: %s/%s",
		    err,
		    fourcc(componentType).svalue,
		    fourcc(componentSubType).svalue);
	    throw std::runtime_error(s);
	}
	return ComponentX(ci);
    }
    operator ComponentInstance() { return m_instance.get(); }

    /* for PropertySupport */
    long _getPropertyInfo(
	    OSType inPropClass,
	    OSType inPropID,
	    OSType *outPropType,
	    ByteCount *outPropValueSize,
	    UInt32 *outPropertyFlags)
    {
	return QTGetComponentPropertyInfo(
		    m_instance.get(),
		    inPropClass,
		    inPropID,
		    outPropType,
		    outPropValueSize,
		    outPropertyFlags);
    }
    long _getProperty(
	    OSType inPropClass,
	    OSType inPropID,
	    ByteCount inPropValueSize,
	    void * outPropValueAddress,
	    ByteCount *outPropValueSizeUsed)
    {
	return QTGetComponentProperty(
		    m_instance.get(),
		    inPropClass,
		    inPropID,
		    inPropValueSize,
		    outPropValueAddress,
		    outPropValueSizeUsed);
    }
    long _setProperty(
	    OSType inPropClass,
	    OSType inPropID,
	    ByteCount inPropValueSize,
	    const void *inPropValueAddress)
    {
	return QTSetComponentProperty(
		    m_instance.get(),
		    inPropClass,
		    inPropID,
		    inPropValueSize,
		    inPropValueAddress);
    }
};

#endif
