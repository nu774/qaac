#ifndef AudioConverterX_H
#define AudioConverterX_H

#include "CoreAudioToolbox.h"

class AudioConverterX {
    x::shared_ptr<OpaqueAudioConverter> m_converter;
public:
    AudioConverterX() {}
    AudioConverterX(AudioConverterRef converter, bool takeOwn)
    {
	attach(converter, takeOwn);
    }
    AudioConverterX(const AudioStreamBasicDescription &iasbd,
	    const AudioStreamBasicDescription &oasbd)
    {
	AudioConverterRef converter;
	CHECKCA(AudioConverterNew(&iasbd, &oasbd, &converter));
	attach(converter, true);
    }
    void attach(AudioConverterRef converter, bool takeOwn)
    {
	OSStatus (*dispose)(AudioConverterRef) =
	    takeOwn ? AudioConverterDispose : fakeDispose;
	m_converter = x::shared_ptr<OpaqueAudioConverter>(converter, dispose);
    }
    operator AudioConverterRef() { return m_converter.get(); }

    // properties
    UInt32 getSampleRateConverterComplexity()
    {
	UInt32 value;
	UInt32 size = sizeof value;
	CHECKCA(AudioConverterGetProperty(m_converter.get(),
		kAudioConverterSampleRateConverterComplexity, &size, &value));
	return value;
    }
    void setSampleRateConverterComplexity(UInt32 complexity)
    {
	CHECKCA(AudioConverterSetProperty(m_converter.get(),
		kAudioConverterSampleRateConverterComplexity,
		sizeof complexity, &complexity));
    }
    UInt32 getSampleRateConverterQuality()
    {
	UInt32 value;
	UInt32 size = sizeof value;
	CHECKCA(AudioConverterGetProperty(m_converter.get(),
		kAudioConverterSampleRateConverterQuality, &size, &value));
	return value;
    }
    void setSampleRateConverterQuality(UInt32 quality)
    {
	CHECKCA(AudioConverterSetProperty(m_converter.get(),
		kAudioConverterSampleRateConverterQuality,
		sizeof quality, &quality));
    }
    UInt32 getPrimeMethod()
    {
	UInt32 value;
	UInt32 size = sizeof value;
	CHECKCA(AudioConverterGetProperty(m_converter.get(),
	    kAudioConverterPrimeMethod, &size, &value));
	return value;
    }
    void getPrimeInfo(AudioConverterPrimeInfo *result)
    {
	UInt32 size = sizeof(AudioConverterPrimeInfo);
	CHECKCA(AudioConverterGetProperty(m_converter.get(),
	    kAudioConverterPrimeInfo, &size, result));
    }
    void getCompressionMagicCookie(std::vector<uint8_t> *result)
    {
	UInt32 size;
	Boolean writable;
	CHECKCA(AudioConverterGetPropertyInfo(m_converter.get(),
		    kAudioConverterCompressionMagicCookie, &size, &writable));
	std::vector<uint8_t> vec(size / sizeof(uint8_t));
	CHECKCA(AudioConverterGetProperty(m_converter.get(),
		kAudioConverterCompressionMagicCookie, &size, &vec[0]));
	result->swap(vec);
    }
    UInt32 getEncodeBitRate()
    {
	UInt32 value;
	UInt32 size = sizeof value;
	CHECKCA(AudioConverterGetProperty(m_converter.get(),
	    kAudioConverterEncodeBitRate, &size, &value));
	return value;
    }
    void setEncodeBitRate(UInt32 value)
    {
	CHECKCA(AudioConverterSetProperty(m_converter.get(),
	    kAudioConverterEncodeBitRate, sizeof value, &value));
    }
    void getInputChannelLayout(x::shared_ptr<AudioChannelLayout> *result)
    {
	UInt32 size;
	Boolean writable;
	CHECKCA(AudioConverterGetPropertyInfo(m_converter.get(),
		    kAudioConverterInputChannelLayout, &size, &writable));
	x::shared_ptr<AudioChannelLayout> acl(
	    reinterpret_cast<AudioChannelLayout*>(std::malloc(size)),
	    std::free);
	CHECKCA(AudioConverterGetProperty(m_converter.get(),
		kAudioConverterInputChannelLayout, &size, acl.get()));
	result->swap(acl);
    }
    void setInputChannelLayout(const AudioChannelLayout *value)
    {
	size_t size = offsetof(AudioChannelLayout,
		mChannelDescriptions[value->mNumberChannelDescriptions]);
	CHECKCA(AudioConverterSetProperty(m_converter.get(),
		kAudioConverterInputChannelLayout, size, value));
    }
    void getOutputChannelLayout(x::shared_ptr<AudioChannelLayout> *result)
    {
	UInt32 size;
	Boolean writable;
	CHECKCA(AudioConverterGetPropertyInfo(m_converter.get(),
		    kAudioConverterOutputChannelLayout, &size, &writable));
	x::shared_ptr<AudioChannelLayout> acl(
	    reinterpret_cast<AudioChannelLayout*>(std::malloc(size)),
	    std::free);
	CHECKCA(AudioConverterGetProperty(m_converter.get(),
		kAudioConverterOutputChannelLayout, &size, acl.get()));
	result->swap(acl);
    }
    void setOutputChannelLayout(const AudioChannelLayout *value)
    {
	size_t size = offsetof(AudioChannelLayout,
		mChannelDescriptions[value->mNumberChannelDescriptions]);
	CHECKCA(AudioConverterSetProperty(m_converter.get(),
		kAudioConverterOutputChannelLayout, size, value));
    }
    void getApplicableEncodeBitRates(std::vector<AudioValueRange> *result)
    {
	UInt32 size;
	Boolean writable;
	CHECKCA(AudioConverterGetPropertyInfo(m_converter.get(),
		    kAudioConverterApplicableEncodeBitRates, &size, &writable));
	std::vector<AudioValueRange> vec(size / sizeof(AudioValueRange));
	CHECKCA(AudioConverterGetProperty(m_converter.get(),
		kAudioConverterApplicableEncodeBitRates, &size, &vec[0]));
	result->swap(vec);
    }
    void getApplicableEncodeSampleRates(std::vector<AudioValueRange> *result)
    {
	UInt32 size;
	Boolean writable;
	CHECKCA(AudioConverterGetPropertyInfo(m_converter.get(),
		    kAudioConverterApplicableEncodeSampleRates,
		    &size, &writable));
	std::vector<AudioValueRange> vec(size / sizeof(AudioValueRange));
	CHECKCA(AudioConverterGetProperty(m_converter.get(),
		kAudioConverterApplicableEncodeSampleRates, &size, &vec[0]));
	result->swap(vec);
    }
    void getInputStreamDescription(AudioStreamBasicDescription *result)
    {
	UInt32 size = sizeof(*result);
	CHECKCA(AudioConverterGetProperty(m_converter.get(),
		    kAudioConverterCurrentInputStreamDescription,
		    &size, result));
    }
    void getOutputStreamDescription(AudioStreamBasicDescription *result)
    {
	UInt32 size = sizeof(*result);
	CHECKCA(AudioConverterGetProperty(m_converter.get(),
		    kAudioConverterCurrentOutputStreamDescription,
		    &size, result));
    }
    UInt32 getMaximumOutputPacketSize()
    {
	UInt32 result;
	UInt32 size = sizeof result;
	CHECKCA(AudioConverterGetProperty(m_converter.get(),
		    kAudioConverterPropertyMaximumOutputPacketSize,
		    &size, &result));
	return result;
    }
    UInt32 getBitRateControlMode()
    {
	UInt32 result;
	UInt32 size = sizeof result;
	CHECKCA(AudioConverterGetProperty(m_converter.get(),
		    kAudioCodecPropertyBitRateControlMode,
		    &size, &result));
	return result;
    }
    void setBitRateControlMode(UInt32 value)
    {
	CHECKCA(AudioConverterSetProperty(m_converter.get(),
		    kAudioCodecPropertyBitRateControlMode,
		    sizeof value, &value));
    }
    UInt32 getSoundQualityForVBR()
    {
	UInt32 result;
	UInt32 size = sizeof result;
	CHECKCA(AudioConverterGetProperty(m_converter.get(),
		    kAudioCodecPropertySoundQualityForVBR,
		    &size, &result));
	return result;
    }
    void setSoundQualityForVBR(UInt32 value)
    {
	CHECKCA(AudioConverterSetProperty(m_converter.get(),
		    kAudioCodecPropertySoundQualityForVBR,
		    sizeof value, &value));
    }
    UInt32 getCodecQuality()
    {
	UInt32 result;
	UInt32 size = sizeof result;
	CHECKCA(AudioConverterGetProperty(m_converter.get(),
		    kAudioConverterCodecQuality, &size, &result));
	return result;
    }
    void setCodecQuality(UInt32 value)
    {
	CHECKCA(AudioConverterSetProperty(m_converter.get(),
		    kAudioConverterCodecQuality, sizeof value, &value));
    }

    // helpers
    double getClosestAvailableBitRate(double value)
    {
	std::vector<AudioValueRange> rates;
	getApplicableEncodeBitRates(&rates);
	double distance = DBL_MAX;
	double pick = 0;
	std::vector<AudioValueRange>::const_iterator it = rates.begin();
	for (; it != rates.end(); ++it) {
	    if (!it->mMinimum) continue;
	    double diff = std::abs(value - it->mMinimum);
	    if (distance > diff) {
		distance = diff;
		pick = it->mMinimum;
	    }
	}
	return pick;
    }
private:
    static OSStatus fakeDispose(AudioConverterRef) { return 0; }
};

#endif
