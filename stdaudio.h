#ifndef STDAUDIO_H
#define STDAUDIO_H

#include <QuickTimeComponents.h>
#include "qthelper.h"
#include "qtmoviehelper.h"

class StdAudioComponentX : public ComponentX {
    enum {
	kSCAudio = kQTPropertyClass_SCAudio
    };
    enum {
	kApplicableChannelLayoutTagList =
	    kQTSCAudioPropertyID_ApplicableChannelLayoutTagList,
	kApplicableChannelLayoutTagNamesList =
	    kQTSCAudioPropertyID_ApplicableChannelLayoutTagNamesList,
	kApplicableLPCMBitsPerChannelList =
	    kQTSCAudioPropertyID_ApplicableLPCMBitsPerChannelList,
	kApplicableSampleRateList =
	    kQTSCAudioPropertyID_ApplicableSampleRateList,
	kAvailableChannelLayoutTagList =
	    kQTSCAudioPropertyID_AvailableChannelLayoutTagList,
	kAvailableChannelLayoutTagNamesList =
	    kQTSCAudioPropertyID_AvailableChannelLayoutTagNamesList,
	kAvailableCompressionFormatList =
	    kQTSCAudioPropertyID_AvailableCompressionFormatList,
	kAvailableCompressionFormatNamesList =
	    kQTSCAudioPropertyID_AvailableCompressionFormatNamesList,
	kAvailableLPCMBitsPerChannelList =
	    kQTSCAudioPropertyID_AvailableLPCMBitsPerChannelList,
	kAvailableSampleRateList =
	    kQTSCAudioPropertyID_AvailableSampleRateList,
	kBasicDescription =
	    kQTSCAudioPropertyID_BasicDescription,
	kBitRate = kQTSCAudioPropertyID_BitRate,
	kChannelLayout = kQTSCAudioPropertyID_ChannelLayout,
	kCodecSpecificSettingsArray =
	    kQTSCAudioPropertyID_CodecSpecificSettingsArray,
	kInputBasicDescription = kQTSCAudioPropertyID_InputBasicDescription,
	kInputChannelLayout = kQTSCAudioPropertyID_InputChannelLayout,
	kInputMagicCookie = kQTSCAudioPropertyID_InputMagicCookie,
	kInputSoundDescription = kQTSCAudioPropertyID_InputSoundDescription,
	kMagicCookie = kQTSCAudioPropertyID_MagicCookie,
	kMaximumOutputPacketSize =
	    kQTSCAudioPropertyID_MaximumOutputPacketSize,
	kOutputFormatIsExternallyFramed =
	    kQTSCAudioPropertyID_OutputFormatIsExternallyFramed,
	kSoundDescription = kQTSCAudioPropertyID_SoundDescription
    };
public:
    StdAudioComponentX(ComponentInstance instance): ComponentX(instance) {}
    StdAudioComponentX()
    {
	ComponentInstance instance;
	TRYE(OpenADefaultComponent(
		    StandardCompressionType,
		    StandardCompressionSubTypeAudio,
		    &instance));
	attach(instance);
    }
    virtual ~StdAudioComponentX() {}
    static StdAudioComponentX OpenDefault()
    {
	return StdAudioComponentX();
    }
    void getAvailableCompressionFormatList(std::vector<UInt32> *result)
    {
	getVectorProperty(
		kSCAudio,
		kAvailableCompressionFormatList,
		result);
    }
    void getAvailableCompressionFormatNamesList(
	    CFArrayT<CFStringRef> *result)
    {
	CFArrayRef ref = getPodProperty<CFArrayRef>(
			    kSCAudio,
			    kAvailableCompressionFormatNamesList);
	CFArrayT<CFStringRef> p(ref);
	result->swap(p);
    }
    void getAvailableSampleRateList(std::vector<AudioValueRange> *result)
    {
	getVectorProperty(
		kSCAudio,
		kAvailableSampleRateList,
		result);
    }
    void getApplicableSampleRateList(std::vector<AudioValueRange> *result)
    {
	getVectorProperty(kSCAudio, kApplicableSampleRateList, result);
    }
    void getInputMagicCookie(std::vector<char> *result)
    {
	getVectorProperty(kSCAudio, kInputMagicCookie, result);
    }
    void setInputMagicCookie(const void *cookie, ByteCount size)
    {
	setProperty(kSCAudio, kInputMagicCookie, size, cookie);
    }
    void getMagicCookie(std::vector<char> *result)
    {
	getVectorProperty(kSCAudio, kMagicCookie, result);
    }
    void setMagicCookie(const void *cookie, ByteCount size)
    {
	setProperty(kSCAudio, kMagicCookie, size, cookie);
    }
    void getAvailableLPCMBitsPerChannelList(std::vector<UInt32> *result)
    {
	getVectorProperty(
		kSCAudio, kAvailableLPCMBitsPerChannelList, result);
    }
    void getApplicableLPCMBitsPerChannelList(std::vector<UInt32> *result)
    {
	getVectorProperty(
		kSCAudio, kApplicableLPCMBitsPerChannelList, result);
    }
    void getInputChannelLayout(AudioChannelLayoutX *result)
    {
	AudioChannelLayoutX::owner_t value;
	getPointerProperty(kSCAudio, kInputChannelLayout, &value);
	*result = value.get();
    }
    void setInputChannelLayout(const AudioChannelLayout *acl)
    {
	ByteCount size = AudioChannelLayout_length(acl);
	setProperty(kSCAudio, kInputChannelLayout, size, acl);
    }
    void getChannelLayout(AudioChannelLayoutX *result)
    {
	AudioChannelLayoutX::owner_t value;
	getPointerProperty(kSCAudio, kChannelLayout, &value);
	*result = value.get();
    }
    void setChannelLayout(const AudioChannelLayout *acl)
    {
	ByteCount size = AudioChannelLayout_length(acl);
	setProperty(kSCAudio, kChannelLayout, size, acl);
    }
    void getAvailableChannelLayoutTagList(std::vector<UInt32> *result)
    {
	getVectorProperty(
		kSCAudio, kAvailableChannelLayoutTagList, result);
    }
    void getAvailableChannelLayoutTagNamesList(
	    CFArrayT<CFStringRef> *result)
    {
	CFArrayRef ref = getPodProperty<CFArrayRef>(
			    kSCAudio,
			    kAvailableChannelLayoutTagNamesList);
	CFArrayT<CFStringRef> p(ref);
	result->swap(p);
    }
    void getApplicableChannelLayoutTagList(std::vector<UInt32> *result)
    {
	getVectorProperty(
		kSCAudio, kApplicableChannelLayoutTagList, result);
    }
    void getApplicableChannelLayoutTagNamesList(
	    CFArrayT<CFStringRef> *result)
    {
	CFArrayRef ref = getPodProperty<CFArrayRef>(
		    kSCAudio, kApplicableChannelLayoutTagNamesList);
	CFArrayT<CFStringRef> p(ref);
	result->swap(p);
    }
    void getInputSoundDescription(SoundDescriptionX *result)
    {
	SoundDescriptionHandle handle;
	getPodProperty(kSCAudio, kInputSoundDescription, &handle);
	*result = SoundDescriptionX(handle);
    }
    void setInputSoundDescription(SoundDescriptionHandle handle)
    {
	setPodProperty(kSCAudio, kInputSoundDescription, handle);
    }
    void getSoundDescription(SoundDescriptionX * result)
    {
	SoundDescriptionHandle handle;
	getPodProperty(kSCAudio, kSoundDescription, &handle);
	*result = SoundDescriptionX(handle);
    }
    void setSoundDescription(SoundDescriptionHandle handle)
    {
	setPodProperty(kSCAudio, kSoundDescription, handle);
    }
    void getInputBasicDescription(AudioStreamBasicDescription *asbd)
    {
	getPodProperty(kSCAudio, kInputBasicDescription, asbd);
    }
    void setInputBasicDescription(const AudioStreamBasicDescription &asbd)
    {
	setPodProperty(kSCAudio, kInputBasicDescription, asbd);
    }
    void getBasicDescription(AudioStreamBasicDescription *asbd)
    {
	getPodProperty(kSCAudio, kBasicDescription, asbd);
    }
    void setBasicDescription(const AudioStreamBasicDescription &asbd)
    {
	setPodProperty(kSCAudio, kBasicDescription, asbd);
    }
    void getCodecSpecificSettingsArray(
	    CFArrayT<CFDictionaryRef> *result)
    {
	CFArrayRef ref;
	getPodProperty(kSCAudio, kCodecSpecificSettingsArray, &ref);
	if (!ref) throw std::runtime_error("getCodecSpecificSettingsArray");
	result->swap(CFArrayT<CFDictionaryRef>(ref));
    }
    void setCodecSpecificSettingsArray(const CFArrayRef &ref)
    {
	setPodProperty(kSCAudio, kCodecSpecificSettingsArray, ref);
    }
    UInt32 getBitRate()
    {
	return getPodProperty<UInt32>(kSCAudio, kBitRate);
    }
    UInt32 getMaximumOutputPacketSize()
    {
	return getPodProperty<UInt32>(kSCAudio, kMaximumOutputPacketSize);
    }
    Boolean getOutputFormatIsExternallyFramed()
    {
	return getPodProperty<Boolean>(
		kSCAudio, kOutputFormatIsExternallyFramed);
    }
};

#endif
