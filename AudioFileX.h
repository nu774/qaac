#ifndef AudioFileX_H
#define AudioFileX_H

#include "AudioFile.h"
#include "CoreAudioHelper.h"

class AudioFileX {
    x::shared_ptr<OpaqueAudioFileID> m_file;
public:
    AudioFileX() {}
    AudioFileX(AudioFileID file, bool takeOwn)
    {
	attach(file, takeOwn);
    }
    void attach(AudioFileID file, bool takeOwn)
    {
	OSStatus (*dispose)(AudioFileID) =
	    takeOwn ? AudioFileClose : fakeDispose;
	m_file = x::shared_ptr<OpaqueAudioFileID>(file, dispose);
    }
    operator AudioFileID() { return m_file.get(); }

    // property accessors
    UInt32 getFileFormat()
    {
	UInt32 value;
	UInt32 size = sizeof value;
	CHECKCA(AudioFileGetProperty(m_file.get(),
		    kAudioFilePropertyFileFormat, &size, &value));
	return value;
    }
    void getDataFormat(AudioStreamBasicDescription *result)
    {
	UInt32 size = sizeof(AudioStreamBasicDescription);
	CHECKCA(AudioFileGetProperty(m_file.get(),
		    kAudioFilePropertyDataFormat, &size, result));
    }
    UInt64 getAudioDataPacketCount()
    {
	UInt64 value;
	UInt32 size = sizeof value;
	CHECKCA(AudioFileGetProperty(m_file.get(),
		    kAudioFilePropertyAudioDataPacketCount,
		    &size, &value));
	return value;
    }
    void getChannelLayout(x::shared_ptr<AudioChannelLayout> *layout)
    {
	UInt32 size;
	UInt32 writable;
	CHECKCA(AudioFileGetPropertyInfo(m_file.get(),
		    kAudioFilePropertyChannelLayout, &size, &writable));
	x::shared_ptr<AudioChannelLayout> acl(
	    reinterpret_cast<AudioChannelLayout*>(std::malloc(size)),
	    std::free);
	CHECKCA(AudioFileGetProperty(m_file.get(),
		kAudioFilePropertyChannelLayout, &size, acl.get()));
	layout->swap(acl);
    }
    void getInfoDictionary(x::shared_ptr<const __CFDictionary> *result)
    {
	UInt32 size = sizeof(CFDictionaryRef);
	CFDictionaryRef dict;
	CHECKCA(AudioFileGetProperty(m_file.get(),
		    kAudioFilePropertyInfoDictionary, &size, &dict));
	x::shared_ptr<const __CFDictionary> ptr(dict, CFRelease);
	result->swap(ptr);
    }

private:
    static OSStatus fakeDispose(AudioFileID file)
    {
	return 0;
    }
};

#endif
