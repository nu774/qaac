#ifndef AudioFileX_H
#define AudioFileX_H

#include "CoreAudio/AudioFile.h"
#include "CoreAudio/AudioFormat.h"
#include "strutil.h"
#include "cautil.h"

namespace afutil {
    inline uint32_t getTypesForExtension(const wchar_t *fname)
    {
	const wchar_t *pos = std::wcsrchr(fname, L'.');
	if (!pos)
	    return 0;
	CFStringPtr cfsp = cautil::W2CF(strutil::wslower(pos + 1));
	CFStringRef cfsr = cfsp.get();
	UInt32 type = 0;
	UInt32 size = sizeof(type);
	CHECKCA(AudioFileGetGlobalInfo(kAudioFileGlobalInfo_TypesForExtension,
				       sizeof(CFStringRef),
				       &cfsr, &size, &type));
	return type;
    }
    inline void getReadableTypes(std::vector<uint32_t> *result)
    {
	UInt32 size;
	CHECKCA(AudioFileGetGlobalInfoSize(kAudioFileGlobalInfo_ReadableTypes,
					   0, 0, &size));
	std::vector<uint32_t> vec(size / sizeof(uint32_t));
	CHECKCA(AudioFileGetGlobalInfo(kAudioFileGlobalInfo_ReadableTypes,
				       0, 0, &size, &vec[0]));
	result->swap(vec);
    }
    inline void getWritableTypes(std::vector<uint32_t> *result)
    {
	UInt32 size;
	CHECKCA(AudioFileGetGlobalInfoSize(kAudioFileGlobalInfo_WritableTypes,
					   0, 0, &size));
	std::vector<uint32_t> vec(size / sizeof(uint32_t));
	CHECKCA(AudioFileGetGlobalInfo(kAudioFileGlobalInfo_WritableTypes,
				       0, 0, &size, &vec[0]));
	result->swap(vec);
    }
    inline std::wstring getFileTypeName(uint32_t type)
    {
	CFStringRef name;
	UInt32 size = sizeof(name);
	CHECKCA(AudioFileGetGlobalInfo(kAudioFileGlobalInfo_FileTypeName,
				       sizeof(UInt32), &type, &size, &name));
	CFStringPtr _(name, CFRelease);
	return cautil::CF2W(name);
    }
    inline void getExtensionsForType(uint32_t type,
				     std::vector<std::wstring> *vec)
    {
	CFArrayRef aref;
	UInt32 size = sizeof(aref);
	CHECKCA(AudioFileGetGlobalInfo(kAudioFileGlobalInfo_ExtensionsForType,
				       sizeof(UInt32), &type, &size, &aref));
	std::shared_ptr<const __CFArray> _(aref, CFRelease);
	CFIndex count = CFArrayGetCount(aref);
	std::vector<std::wstring> result;
	for (CFIndex i = 0; i < count; ++i) {
	    CFStringRef value =
		static_cast<CFStringRef>(CFArrayGetValueAtIndex(aref, i));
	    result.push_back(cautil::CF2W(value));
	}
	vec->swap(result);
    }
    inline void getAvailableFormatIDs(uint32_t type,
				      std::vector<uint32_t> *result)
    {
	UInt32 size;
	CHECKCA(AudioFileGetGlobalInfoSize(
		   kAudioFileGlobalInfo_AvailableFormatIDs,
		   sizeof(type), &type, &size));
	std::vector<uint32_t> vec(size / sizeof(uint32_t));
	CHECKCA(AudioFileGetGlobalInfo(kAudioFileGlobalInfo_AvailableFormatIDs,
				       sizeof(type), &type, &size, &vec[0]));
	result->swap(vec);
    }
    inline std::wstring
	getASBDFormatName(const AudioStreamBasicDescription &asbd)
    {
	CFStringRef s;
	UInt32 size = sizeof(s);
	CHECKCA(AudioFormatGetProperty(kAudioFormatProperty_FormatName,
				       sizeof(asbd), &asbd,
				       &size, &s));
	CFStringPtr _(s, CFRelease);
	std::wstring ws = cautil::CF2W(s);
	// XXX
	// Workaround for CoreAudio bug. MPEG Layer 1 and 2 is reported as
	// Layer 3
	if (asbd.mFormatID == '.mp1' || asbd.mFormatID == '.mp2') {
	    const wchar_t *p;
	    if ((p = std::wcsstr(ws.c_str(), L"Layer 3")) != 0)
		ws[p - ws.c_str() + 6] = asbd.mFormatID & 0xff;
	}
	return ws;
    }
    inline void id3TagToDictinary(const void *data, size_t size,
				  CFDictionaryPtr *dict)
    {
	UInt32 k = kAudioFormatProperty_ID3TagToDictionary;
	CFDictionaryRef dref;
	UInt32 osize = sizeof dref;
	CHECKCA(AudioFormatGetProperty(k, size, data, &osize, &dref));
	CFDictionaryPtr dp(dref, CFRelease);
	dict->swap(dp);
    }
    inline void getChannelLayoutForTag(uint32_t tag,
				       std::shared_ptr<AudioChannelLayout> *p)
    {
	UInt32 k = kAudioFormatProperty_ChannelLayoutForTag;
	UInt32 size;
	CHECKCA(AudioFormatGetPropertyInfo(k, sizeof(tag), &tag, &size));
	std::shared_ptr<AudioChannelLayout>
	    acl(reinterpret_cast<AudioChannelLayout*>(std::malloc(size)),
		std::free);
	CHECKCA(AudioFormatGetProperty(k, sizeof(tag), &tag, &size, acl.get()));
	p->swap(acl);
    }
}

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
    void close() { m_file.reset(); }

    // property accessors
    uint32_t getFileFormat()
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
    uint64_t getAudioDataPacketCount()
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
    void setPacketTableInfo(const AudioFilePacketTableInfo &info)
    {
	CHECKCA(AudioFileSetProperty(m_file.get(),
				     kAudioFilePropertyPacketTableInfo,
				     sizeof(AudioFilePacketTableInfo),
				     &info));
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
    void setChannelLayout(const AudioChannelLayout &layout)
    {
	UInt32 size = cautil::sizeofAudioChannelLayout(layout);
	CHECKCA(AudioFileSetProperty(m_file.get(),
				     kAudioFilePropertyChannelLayout,
				     size,
				     &layout));
    }
    void getMagicCookieData(std::vector<uint8_t> *cookie)
    {
	UInt32 size;
	UInt32 writable;
	CHECKCA(AudioFileGetPropertyInfo(m_file.get(),
					 kAudioFilePropertyMagicCookieData,
					 &size, &writable));
	if (size > 0) {
	    std::vector<uint8_t> vec(size);
	    CHECKCA(AudioFileGetProperty(m_file.get(),
					 kAudioFilePropertyMagicCookieData,
					 &size, &vec[0]));
	    cookie->swap(vec);
	} else {
	    cookie->clear();
	}
    }
    void setMagicCookieData(const uint8_t *cookie, size_t size)
    {
	UInt32 len = size;
	CHECKCA(AudioFileSetProperty(m_file.get(),
				     kAudioFilePropertyMagicCookieData,
				     len,
				     cookie));
    }
    void setReserveDuration(double duration)
    {
	CHECKCA(AudioFileSetProperty(m_file.get(),
				     kAudioFilePropertyReserveDuration,
				     sizeof(duration),
				     &duration));
    }
    uint32_t getPacketSizeUpperBound()
    {
	UInt32 len;
	UInt32 size = sizeof(len);
	CHECKCA(AudioFileGetProperty(m_file.get(),
				     kAudioFilePropertyPacketSizeUpperBound,
				     &size, &len));
	return len;
    }
    uint32_t getMaximumPacketSize()
    {
	UInt32 len;
	UInt32 size = sizeof(len);
	CHECKCA(AudioFileGetProperty(m_file.get(),
				     kAudioFilePropertyMaximumPacketSize,
				     &size, &len));
	return len;
    }
    uint64_t getAudioDataByteCount()
    {
	UInt64 len;
	UInt32 size = sizeof(len);
	CHECKCA(AudioFileGetProperty(m_file.get(),
				     kAudioFilePropertyAudioDataByteCount,
				     &size, &len));
	return len;
    }
    void getInfoDictionary(CFDictionaryPtr *dict)
    {
	CFDictionaryRef dictref;
	UInt32 size = sizeof dict;
	CHECKCA(AudioFileGetProperty(m_file.get(),
		    kAudioFilePropertyInfoDictionary,
		    &size, &dictref));
	CFDictionaryPtr newdic(dictref, CFRelease);
	dict->swap(newdic);
    }
    void setInfoDictionary(CFDictionaryRef dict)
    {
	CHECKCA(AudioFileSetProperty(m_file.get(),
				     kAudioFilePropertyInfoDictionary,
				     sizeof(dict), &dict));
    }
    void getUserData(uint32_t fcc, uint32_t index, std::vector<uint8_t> *res)
    {
	UInt32 size;
	CHECKCA(AudioFileGetUserDataSize(m_file.get(), fcc, index, &size));
	std::vector<uint8_t> vec(size);
	CHECKCA(AudioFileGetUserData(m_file.get(), fcc, index, &size,
				     &vec[0]));
	res->swap(vec);
    }
    void setUserData(uint32_t fcc, uint32_t index, const void *data,
		     size_t size)
    {
	CHECKCA(AudioFileSetUserData(m_file.get(), fcc, index, size, data));
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
    uint32_t getBitrate()
    {
	UInt32 bitrate;
	UInt32 size = sizeof(bitrate);
	CHECKCA(AudioFileGetProperty(m_file.get(),
				     kAudioFilePropertyBitRate,
				     &size, &bitrate));
	return bitrate;
    }
    int64_t getPacketToByte(int64_t packet)
    {
	AudioBytePacketTranslation trans = { 0 };
	trans.mPacket = packet;
	UInt32 size = sizeof trans;
	CHECKCA(AudioFileGetProperty(m_file.get(),
				     kAudioFilePropertyPacketToByte,
				     &size, &trans));
	return trans.mByte;
    }
};

#endif
