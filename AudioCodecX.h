#ifndef AudioCodecX_H
#define AudioCodecX_H

#include "CoreAudioToolbox.h"

class AudioCodecX {
    x::shared_ptr<ComponentInstanceRecord> m_codec;
public:
    AudioCodecX() {}
    AudioCodecX(OSType codec)
    {
	AudioComponentDescription desc = { 'aenc', codec, 0 };
	AudioComponent component = AudioComponentFindNext(0, &desc);
	if (!component) {
	    throw std::runtime_error(format(
		    "AudioComponentFindNext: component %s not found",
		    fourcc(codec).svalue));
	}
	AudioComponentInstance aci;
	CHECKCA(AudioComponentInstanceNew(component, &aci));
	attach(reinterpret_cast<AudioCodec>(aci), true);
    }
    AudioCodecX(AudioCodec codec, bool takeOwn)
    {
	attach(codec, takeOwn);
    }
    void attach(AudioCodec codec, bool takeOwn)
    {
	OSStatus (*dispose)(AudioCodec) =
	    takeOwn ? reinterpret_cast<OSStatus (*)(AudioCodec)>(
			AudioComponentInstanceDispose)
		    : fakeDispose;
	m_codec = x::shared_ptr<ComponentInstanceRecord>(codec, dispose);
    }
    operator AudioCodec() { return m_codec.get(); }

    // property accessor
    void getAvailableInputSampleRates(std::vector<AudioValueRange> *result)
    {
	UInt32 size;
	Boolean writable;
	CHECKCA(AudioCodecGetPropertyInfo(m_codec.get(),
		kAudioCodecPropertyAvailableInputSampleRates,
		&size, &writable));
	std::vector<AudioValueRange> vec(size / sizeof(AudioValueRange));
	CHECKCA(AudioCodecGetProperty(m_codec.get(),
		kAudioCodecPropertyAvailableInputSampleRates,
		&size, &vec[0]));
	result->swap(vec);
    }
    void getAvailableOutputSampleRates(std::vector<AudioValueRange> *result)
    {
	UInt32 size;
	Boolean writable;
	CHECKCA(AudioCodecGetPropertyInfo(m_codec.get(),
		kAudioCodecPropertyAvailableOutputSampleRates,
		&size, &writable));
	std::vector<AudioValueRange> vec(size / sizeof(AudioValueRange));
	CHECKCA(AudioCodecGetProperty(m_codec.get(),
		kAudioCodecPropertyAvailableOutputSampleRates,
		&size, &vec[0]));
	result->swap(vec);
    }
    void getAvailableOutputChannelLayoutTags(std::vector<UInt32> *result)
    {
	UInt32 size;
	Boolean writable;
	CHECKCA(AudioCodecGetPropertyInfo(m_codec.get(),
		    kAudioCodecPropertyAvailableOutputChannelLayoutTags,
		    &size, &writable));
	std::vector<UInt32> vec(size/sizeof(UInt32));
	CHECKCA(AudioCodecGetProperty(m_codec.get(),
		kAudioCodecPropertyAvailableOutputChannelLayoutTags,
		&size, &vec[0]));
	result->swap(vec);
    }
    bool getIsInitialized()
    {
	UInt32 value;
	UInt32 size = sizeof value;
	CHECKCA(AudioCodecGetProperty(m_codec.get(),
		kAudioCodecPropertyIsInitialized, &size, &value));
	return !!value;
    }
    void getApplicableBitRateRange(std::vector<AudioValueRange> *result)
    {
	UInt32 size;
	Boolean writable;
	CHECKCA(AudioCodecGetPropertyInfo(m_codec.get(),
		kAudioCodecPropertyApplicableBitRateRange,
		&size, &writable));
	std::vector<AudioValueRange> vec(size / sizeof(AudioValueRange));
	CHECKCA(AudioCodecGetProperty(m_codec.get(),
		kAudioCodecPropertyApplicableBitRateRange, &size, &vec[0]));
	result->swap(vec);
    }

    // helpers
    bool isAvailableOutputChannelLayout(UInt32 tag)
    {
	std::vector<UInt32> tags;
	getAvailableOutputChannelLayoutTags(&tags);
	return std::find(tags.begin(), tags.end(), tag) != tags.end();
    }

    double getClosestAvailableOutputSampleRate(double value)
    {
	std::vector<AudioValueRange> rates;
	getAvailableOutputSampleRates(&rates);
	double distance = DBL_MAX;
	double pick = value;
	std::vector<AudioValueRange>::const_iterator it = rates.begin();
	for (; it != rates.end(); ++it) {
	    if (!it->mMinimum && !it->mMaximum)
		continue;
	    if (it->mMinimum <= value && value <= it->mMaximum)
		return value;
	    double ldiff = std::fabs(value - it->mMinimum),
		   rdiff = std::fabs(value - it->mMaximum);
	    double diff = std::min(ldiff, rdiff);
	    if (distance > diff) {
		distance = diff;
		pick = ldiff > rdiff ? it->mMaximum : it->mMinimum;
	    }
	}
	return pick;
    }
private:
    static OSStatus fakeDispose(AudioCodec) { return 0; }
};

#endif
