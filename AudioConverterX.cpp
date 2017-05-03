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
    struct F {
        static OSStatus dispose(AudioConverterRef) { return 0; }
    };
    m_converter.reset(converter, takeOwn ? AudioConverterDispose : F::dispose);
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
void AudioConverterX::getPrimeInfo(AudioConverterPrimeInfo *result)
{
    UInt32 size = sizeof(AudioConverterPrimeInfo);
    CHECKCA(AudioConverterGetProperty(m_converter.get(),
                                      kAudioConverterPrimeInfo, &size, result));
}
void AudioConverterX::setPrimeInfo(const AudioConverterPrimeInfo &info)
{
    CHECKCA(AudioConverterSetProperty(m_converter.get(),
                                      kAudioConverterPrimeInfo,
                                      sizeof(info), &info));
}
void AudioConverterX::getCompressionMagicCookie(std::vector<uint8_t> *result)
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
void AudioConverterX::
setDecompressionMagicCookie(const std::vector<uint8_t> &cookie)
{
    CHECKCA(AudioConverterSetProperty(m_converter.get(),
            kAudioConverterDecompressionMagicCookie,
            cookie.size(), cookie.data()));
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
void AudioConverterX::
getInputChannelLayout(std::shared_ptr<AudioChannelLayout> *result)
{
    UInt32 size;
    Boolean writable;
    CHECKCA(AudioConverterGetPropertyInfo(m_converter.get(),
                kAudioConverterInputChannelLayout, &size, &writable));
    std::shared_ptr<AudioChannelLayout>
        acl(reinterpret_cast<AudioChannelLayout*>(std::malloc(size)),
            std::free);
    CHECKCA(AudioConverterGetProperty(m_converter.get(),
            kAudioConverterInputChannelLayout, &size, acl.get()));
    result->swap(acl);
}
void AudioConverterX::setInputChannelLayout(const AudioChannelLayout &value)
{
    UInt32 size = cautil::sizeofAudioChannelLayout(value);
    CHECKCA(AudioConverterSetProperty(m_converter.get(),
            kAudioConverterInputChannelLayout, size, &value));
}
void AudioConverterX::
getOutputChannelLayout(std::shared_ptr<AudioChannelLayout> *result)
{
    UInt32 size;
    Boolean writable;
    CHECKCA(AudioConverterGetPropertyInfo(m_converter.get(),
                kAudioConverterOutputChannelLayout, &size, &writable));
    std::shared_ptr<AudioChannelLayout> acl(
        reinterpret_cast<AudioChannelLayout*>(std::malloc(size)),
        std::free);
    CHECKCA(AudioConverterGetProperty(m_converter.get(),
            kAudioConverterOutputChannelLayout, &size, acl.get()));
    result->swap(acl);
}
void AudioConverterX::setOutputChannelLayout(const AudioChannelLayout &value)
{
    UInt32 size = cautil::sizeofAudioChannelLayout(value);
    CHECKCA(AudioConverterSetProperty(m_converter.get(),
            kAudioConverterOutputChannelLayout, size, &value));
}
void AudioConverterX::
getApplicableEncodeBitRates(std::vector<AudioValueRange> *result)
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
void AudioConverterX::
getApplicableEncodeSampleRates(std::vector<AudioValueRange> *result)
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
void AudioConverterX::
getInputStreamDescription(AudioStreamBasicDescription *result)
{
    UInt32 size = sizeof(*result);
    CHECKCA(AudioConverterGetProperty(m_converter.get(),
                kAudioConverterCurrentInputStreamDescription,
                &size, result));
}
void AudioConverterX::
getOutputStreamDescription(AudioStreamBasicDescription *result)
{
    UInt32 size = sizeof(*result);
    CHECKCA(AudioConverterGetProperty(m_converter.get(),
                kAudioConverterCurrentOutputStreamDescription,
                &size, result));
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

// helpers
double AudioConverterX::getClosestAvailableBitRate(double value)
{
    std::vector<AudioValueRange> rates;
    getApplicableEncodeBitRates(&rates);
    double distance = std::numeric_limits<double>::max();
    double pick = 0;
    for (auto it = rates.begin(); it != rates.end(); ++it) {
        if (!it->mMinimum) continue;
        double diff = std::abs(value - it->mMinimum);
        if (distance > diff) {
            distance = diff;
            pick = it->mMinimum;
        }
    }
    return pick;
}

std::string AudioConverterX::getConfigAsString()
{
    std::string s;
    AudioStreamBasicDescription asbd;
    getOutputStreamDescription(&asbd);
    UInt32 codec = asbd.mFormatID;
    if (codec == 'aac ')
        s = "AAC-LC Encoder";
    else if (codec == 'aach')
        s = "AAC-HE Encoder";
    else
        s = "Apple Lossless Encoder";
    if (codec != 'aac ' && codec != 'aach')
        return s;
    UInt32 value = getBitRateControlMode();
    const char * strategies[] = { "CBR", "ABR", "CVBR", "TVBR" };
    s += strutil::format(", %s", strategies[value]);
    if (value == kAudioCodecBitRateControlMode_Variable) {
        value = getSoundQualityForVBR();
        s += strutil::format(" q%d", value);
    } else {
        value = getEncodeBitRate();
        s += strutil::format(" %gkbps", value / 1000.0);
    }
    value = getCodecQuality();
    s += strutil::format(", Quality %d", value);
    return s;
}

