#ifndef AudioCodecX_H
#define AudioCodecX_H

#include "CoreAudio/AudioCodec.h"
#include "cautil.h"

class AudioCodecX {
    std::shared_ptr<ComponentInstanceRecord> m_codec;
public:
    AudioCodecX() {}
    AudioCodecX(OSType codec)
    {
        AudioComponentDescription desc = { 'aenc', codec, 0 };
        AudioComponent component = AudioComponentFindNext(0, &desc);
        if (!component)
            throw std::runtime_error("AudioComponentFindNext(): "
                                     "codec not found");
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
        auto dispose = [](AudioCodec x) {
            auto y = reinterpret_cast<AudioComponentInstance>(x);
            AudioComponentInstanceDispose(y);
        };
        if (takeOwn)
            m_codec.reset(codec, dispose);
        else
            m_codec.reset(codec, [](AudioCodec) {});
    }
    operator AudioCodec() { return m_codec.get(); }

    // property accessor
    std::vector<AudioValueRange> getAvailableInputSampleRates()
    {
        UInt32 size;
        Boolean writable;
        CHECKCA(AudioCodecGetPropertyInfo(m_codec.get(),
                kAudioCodecPropertyAvailableInputSampleRates,
                &size, &writable));
        std::vector<AudioValueRange> vec(size / sizeof(AudioValueRange));
        CHECKCA(AudioCodecGetProperty(m_codec.get(),
                kAudioCodecPropertyAvailableInputSampleRates,
                &size, vec.data()));
        return vec;
    }
    std::vector<AudioValueRange> getAvailableOutputSampleRates()
    {
        UInt32 size;
        Boolean writable;
        CHECKCA(AudioCodecGetPropertyInfo(m_codec.get(),
                kAudioCodecPropertyAvailableOutputSampleRates,
                &size, &writable));
        std::vector<AudioValueRange> vec(size / sizeof(AudioValueRange));
        CHECKCA(AudioCodecGetProperty(m_codec.get(),
                kAudioCodecPropertyAvailableOutputSampleRates,
                &size, vec.data()));
        return vec;
    }
    std::vector<UInt32> getAvailableOutputChannelLayoutTags()
    {
        UInt32 size;
        Boolean writable;
        CHECKCA(AudioCodecGetPropertyInfo(m_codec.get(),
                    kAudioCodecPropertyAvailableOutputChannelLayoutTags,
                    &size, &writable));
        std::vector<UInt32> vec(size/sizeof(UInt32));
        CHECKCA(AudioCodecGetProperty(m_codec.get(),
                kAudioCodecPropertyAvailableOutputChannelLayoutTags,
                &size, vec.data()));
        return vec;
    }
    bool getIsInitialized()
    {
        UInt32 value;
        UInt32 size = sizeof value;
        CHECKCA(AudioCodecGetProperty(m_codec.get(),
                kAudioCodecPropertyIsInitialized, &size, &value));
        return !!value;
    }
    std::vector<AudioValueRange> getApplicableBitRateRange()
    {
        UInt32 size;
        Boolean writable;
        CHECKCA(AudioCodecGetPropertyInfo(m_codec.get(),
                kAudioCodecPropertyApplicableBitRateRange,
                &size, &writable));
        std::vector<AudioValueRange> vec(size / sizeof(AudioValueRange));
        CHECKCA(AudioCodecGetProperty(m_codec.get(),
                kAudioCodecPropertyApplicableBitRateRange, &size, vec.data()));
        return vec;
    }

    // helpers
    bool isAvailableOutputChannelLayout(UInt32 tag)
    {
        auto tags = getAvailableOutputChannelLayoutTags();
        return std::find(tags.begin(), tags.end(), tag) != tags.end();
    }

    double getClosestAvailableOutputSampleRate(double value)
    {
        auto rates = getAvailableOutputSampleRates();
        double distance = DBL_MAX;
        double pick = value;
        for (auto it = rates.begin(); it != rates.end(); ++it) {
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
};

#endif
