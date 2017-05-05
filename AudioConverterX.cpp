#include "AudioConverterX.h"
#include <limits>
#include <cmath>
#include "CoreAudio/AudioCodec.h"
#include "cautil.h"

AudioConverterX::AudioConverterX(const AudioStreamBasicDescription &iasbd,
                                 const AudioStreamBasicDescription &oasbd)
{
    AudioConverterRef converter;
    CHECKCA(AudioConverterNew(&iasbd, &oasbd, &converter));
    attach(converter, true);
}
void AudioConverterX::attach(AudioConverterRef converter, bool takeOwn)
{
    if (takeOwn)
        m_converter.reset(converter, AudioConverterDispose);
    else
        m_converter.reset(converter, [](AudioConverterRef){});
}

UInt32 AudioConverterX::getSampleRateConverterComplexity()
{
    UInt32 value;
    UInt32 size = sizeof value;
    CHECKCA(AudioConverterGetProperty(m_converter.get(),
            kAudioConverterSampleRateConverterComplexity, &size, &value));
    return value;
}
void AudioConverterX::setSampleRateConverterComplexity(UInt32 complexity)
{
    CHECKCA(AudioConverterSetProperty(m_converter.get(),
            kAudioConverterSampleRateConverterComplexity,
            sizeof complexity, &complexity));
}
UInt32 AudioConverterX::getSampleRateConverterQuality()
{
    UInt32 value;
    UInt32 size = sizeof value;
    CHECKCA(AudioConverterGetProperty(m_converter.get(),
                                      kAudioConverterSampleRateConverterQuality,
                                      &size, &value));
    return value;
}
void AudioConverterX::setSampleRateConverterQuality(UInt32 quality)
{
    CHECKCA(AudioConverterSetProperty(m_converter.get(),
                                      kAudioConverterSampleRateConverterQuality,
                                      sizeof quality, &quality));
}
UInt32 AudioConverterX::getPrimeMethod()
{
    UInt32 value;
    UInt32 size = sizeof value;
    CHECKCA(AudioConverterGetProperty(m_converter.get(),
                                      kAudioConverterPrimeMethod,
                                      &size, &value));
    return value;
}
void AudioConverterX::setPrimeMethod(UInt32 value)
{
    CHECKCA(AudioConverterSetProperty(m_converter.get(),
                                      kAudioConverterPrimeMethod,
                                      sizeof value, &value));
}
AudioConverterPrimeInfo AudioConverterX::getPrimeInfo()
{
    AudioConverterPrimeInfo result;
    UInt32 size = sizeof(AudioConverterPrimeInfo);
    CHECKCA(AudioConverterGetProperty(m_converter.get(),
                                      kAudioConverterPrimeInfo,
                                      &size, &result));
    return result;
}
void AudioConverterX::setPrimeInfo(const AudioConverterPrimeInfo &info)
{
    CHECKCA(AudioConverterSetProperty(m_converter.get(),
                                      kAudioConverterPrimeInfo,
                                      sizeof(info), &info));
}
std::vector<uint8_t> AudioConverterX::getCompressionMagicCookie()
{
    UInt32 size;
    Boolean writable;
    CHECKCA(AudioConverterGetPropertyInfo(m_converter.get(),
                kAudioConverterCompressionMagicCookie, &size, &writable));
    std::vector<uint8_t> vec(size / sizeof(uint8_t));
    CHECKCA(AudioConverterGetProperty(m_converter.get(),
            kAudioConverterCompressionMagicCookie, &size, vec.data()));
    return vec;
}
void AudioConverterX::setDecompressionMagicCookie(const std::vector<uint8_t> &v)
{
    CHECKCA(AudioConverterSetProperty(m_converter.get(),
            kAudioConverterDecompressionMagicCookie,
            v.size(), v.data()));
}
UInt32 AudioConverterX::getEncodeBitRate()
{
    UInt32 value;
    UInt32 size = sizeof value;
    CHECKCA(AudioConverterGetProperty(m_converter.get(),
        kAudioConverterEncodeBitRate, &size, &value));
    return value;
}
void AudioConverterX::setEncodeBitRate(UInt32 value)
{
    CHECKCA(AudioConverterSetProperty(m_converter.get(),
                                      kAudioConverterEncodeBitRate,
                                      sizeof value, &value));
}
std::shared_ptr<AudioChannelLayout> AudioConverterX::getInputChannelLayout()
{
    UInt32 size;
    Boolean writable;
    CHECKCA(AudioConverterGetPropertyInfo(m_converter.get(),
                kAudioConverterInputChannelLayout, &size, &writable));
    std::shared_ptr<AudioChannelLayout>
        acl(static_cast<AudioChannelLayout*>(std::malloc(size)),
            std::free);
    CHECKCA(AudioConverterGetProperty(m_converter.get(),
            kAudioConverterInputChannelLayout, &size, acl.get()));
    return acl;
}
void AudioConverterX::setInputChannelLayout(const AudioChannelLayout &value)
{
    UInt32 size = cautil::sizeofAudioChannelLayout(value);
    CHECKCA(AudioConverterSetProperty(m_converter.get(),
            kAudioConverterInputChannelLayout, size, &value));
}
std::shared_ptr<AudioChannelLayout> AudioConverterX::getOutputChannelLayout()
{
    UInt32 size;
    Boolean writable;
    CHECKCA(AudioConverterGetPropertyInfo(m_converter.get(),
                kAudioConverterOutputChannelLayout, &size, &writable));
    std::shared_ptr<AudioChannelLayout> acl(
        static_cast<AudioChannelLayout*>(std::malloc(size)),
        std::free);
    CHECKCA(AudioConverterGetProperty(m_converter.get(),
            kAudioConverterOutputChannelLayout, &size, acl.get()));
    return acl;
}
void AudioConverterX::setOutputChannelLayout(const AudioChannelLayout &value)
{
    UInt32 size = cautil::sizeofAudioChannelLayout(value);
    CHECKCA(AudioConverterSetProperty(m_converter.get(),
            kAudioConverterOutputChannelLayout, size, &value));
}
std::vector<AudioValueRange> AudioConverterX::getApplicableEncodeBitRates()
{
    UInt32 size;
    Boolean writable;
    CHECKCA(AudioConverterGetPropertyInfo(m_converter.get(),
                kAudioConverterApplicableEncodeBitRates, &size, &writable));
    std::vector<AudioValueRange> vec(size / sizeof(AudioValueRange));
    CHECKCA(AudioConverterGetProperty(m_converter.get(),
            kAudioConverterApplicableEncodeBitRates, &size, vec.data()));
    return vec;
}
std::vector<AudioValueRange> AudioConverterX::getApplicableEncodeSampleRates()
{
    UInt32 size;
    Boolean writable;
    CHECKCA(AudioConverterGetPropertyInfo(m_converter.get(),
                kAudioConverterApplicableEncodeSampleRates,
                &size, &writable));
    std::vector<AudioValueRange> vec(size / sizeof(AudioValueRange));
    CHECKCA(AudioConverterGetProperty(m_converter.get(),
            kAudioConverterApplicableEncodeSampleRates, &size, vec.data()));
    return vec;
}
AudioStreamBasicDescription AudioConverterX::getInputStreamDescription()
{
    AudioStreamBasicDescription result;
    UInt32 size = sizeof(result);
    CHECKCA(AudioConverterGetProperty(m_converter.get(),
                kAudioConverterCurrentInputStreamDescription,
                &size, &result));
    return result;
}
AudioStreamBasicDescription AudioConverterX::getOutputStreamDescription()
{
    AudioStreamBasicDescription result;
    UInt32 size = sizeof(result);
    CHECKCA(AudioConverterGetProperty(m_converter.get(),
                kAudioConverterCurrentOutputStreamDescription,
                &size, &result));
    return result;
}
UInt32 AudioConverterX::getMaximumOutputPacketSize()
{
    UInt32 result;
    UInt32 size = sizeof result;
    CHECKCA(AudioConverterGetProperty(m_converter.get(),
                kAudioConverterPropertyMaximumOutputPacketSize,
                &size, &result));
    return result;
}
UInt32 AudioConverterX::getBitRateControlMode()
{
    UInt32 result;
    UInt32 size = sizeof result;
    CHECKCA(AudioConverterGetProperty(m_converter.get(),
                kAudioCodecPropertyBitRateControlMode,
                &size, &result));
    return result;
}
void AudioConverterX::setBitRateControlMode(UInt32 value)
{
    CHECKCA(AudioConverterSetProperty(m_converter.get(),
                kAudioCodecPropertyBitRateControlMode,
                sizeof value, &value));
}
UInt32 AudioConverterX::getSoundQualityForVBR()
{
    UInt32 result;
    UInt32 size = sizeof result;
    CHECKCA(AudioConverterGetProperty(m_converter.get(),
                kAudioCodecPropertySoundQualityForVBR,
                &size, &result));
    return result;
}
void AudioConverterX::setSoundQualityForVBR(UInt32 value)
{
    CHECKCA(AudioConverterSetProperty(m_converter.get(),
                kAudioCodecPropertySoundQualityForVBR,
                sizeof value, &value));
}
UInt32 AudioConverterX::getCodecQuality()
{
    UInt32 result;
    UInt32 size = sizeof result;
    CHECKCA(AudioConverterGetProperty(m_converter.get(),
                kAudioConverterCodecQuality, &size, &result));
    return result;
}
void AudioConverterX::setCodecQuality(UInt32 value)
{
    CHECKCA(AudioConverterSetProperty(m_converter.get(),
                kAudioConverterCodecQuality, sizeof value, &value));
}
