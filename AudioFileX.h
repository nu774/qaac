#ifndef AudioFileX_H
#define AudioFileX_H

#include "CoreAudio/AudioFile.h"
#include "CoreAudioHelper.h"

class AudioFileX {
    std::shared_ptr<OpaqueAudioFileID> m_file;
public:
    AudioFileX() {}
    AudioFileX(AudioFileID file, bool takeOwn)
    {
	attach(file, takeOwn);
    }
    void attach(AudioFileID file, bool takeOwn)
    {
	struct F {
	    static OSStatus dispose(AudioFileID af) { return 0; }
	};
	m_file.reset(file, takeOwn ? AudioFileClose : F::dispose);
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
    void getPacketTableInfo(AudioFilePacketTableInfo *result)
    {
	UInt32 size = sizeof(AudioFilePacketTableInfo);
	CHECKCA(AudioFileGetProperty(m_file.get(),
				     kAudioFilePropertyPacketTableInfo,
				     &size, result));
    }
    void setPacketTableInfo(const AudioFilePacketTableInfo *info)
    {
	CHECKCA(AudioFileSetProperty(m_file.get(),
				     kAudioFilePropertyPacketTableInfo,
				     sizeof(AudioFilePacketTableInfo),
				     info));
    }
    void getChannelLayout(std::shared_ptr<AudioChannelLayout> *layout)
    {
	UInt32 size;
	UInt32 writable;
	CHECKCA(AudioFileGetPropertyInfo(m_file.get(),
		    kAudioFilePropertyChannelLayout, &size, &writable));
	std::shared_ptr<AudioChannelLayout> acl(
	    reinterpret_cast<AudioChannelLayout*>(std::malloc(size)),
	    std::free);
	CHECKCA(AudioFileGetProperty(m_file.get(),
		kAudioFilePropertyChannelLayout, &size, acl.get()));
	layout->swap(acl);
    }
    void getMagicCookieData(std::vector<uint8_t> *cookie)
    {
	UInt32 size;
	UInt32 writable;
	CHECKCA(AudioFileGetPropertyInfo(m_file.get(),
					 kAudioFilePropertyMagicCookieData,
					 &size, &writable));
	std::vector<uint8_t> vec(size);
	CHECKCA(AudioFileGetProperty(m_file.get(),
				     kAudioFilePropertyMagicCookieData,
				     &size, &vec[0]));
	cookie->swap(vec);
    }
    CFDictionaryRef getInfoDictionary()
    {
	CFDictionaryRef dict;
	UInt32 size = sizeof dict;
	CHECKCA(AudioFileGetProperty(m_file.get(),
		    kAudioFilePropertyInfoDictionary,
		    &size, &dict));
	return dict;
    }
    void getFormatList(std::vector<AudioFormatListItem> *result)
    {
	UInt32 size;
	UInt32 writable;
	CHECKCA(AudioFileGetPropertyInfo(m_file.get(),
					 kAudioFilePropertyFormatList,
					 &size, &writable));
	size_t count = size / sizeof(AudioFormatListItem);
	std::vector<AudioFormatListItem> vec(count);
	CHECKCA(AudioFileGetProperty(m_file.get(),
				     kAudioFilePropertyFormatList,
				     &size, &vec[0]));
	result->swap(vec);
    }
};

#endif
