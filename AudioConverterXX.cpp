#include "AudioConverterXX.h"
#include <limits>
#include <cmath>
#include "CoreAudio/AudioCodec.h"
#include "CoreAudio/AudioComponent.h"
#include "cautil.h"
#include "chanmap.h"

const UInt32 AAC_7_1_Rear_Tag = kAudioChannelLayoutTag_AAC_7_1_B;

std::vector<uint8_t> AudioConverterXX::getCompressionMagicCookie()
{
    auto cookie = BaseT::getCompressionMagicCookie();
    if (isOutputAAC() && m_OutputChannelLayoutTag == AAC_7_1_Rear_Tag) {
        auto asc = cautil::parseMagicCookieAAC(cookie);
        cautil::insert71RearPCEToASC(&asc);
        cautil::replaceASCInMagicCookie(&cookie, asc);
    }
    return cookie;
}

std::shared_ptr<AudioChannelLayout> AudioConverterXX::getInputChannelLayout()
{
    if (!isOutputAAC() || m_InputChannelLayoutTag != AAC_7_1_Rear_Tag)
        return BaseT::getInputChannelLayout();
    else {
        auto acl =
            std::shared_ptr<AudioChannelLayout>(new AudioChannelLayout);
        memset(acl.get(), 0, sizeof(AudioChannelLayout));
        acl->mChannelLayoutTag = m_InputChannelLayoutTag;
        return acl;
    }
}

void AudioConverterXX::setInputChannelLayout(const AudioChannelLayout &acl)
{
    if (!isOutputAAC() || acl.mChannelLayoutTag != AAC_7_1_Rear_Tag)
        BaseT::setInputChannelLayout(acl);
    else {
        m_InputChannelLayoutTag = acl.mChannelLayoutTag;
        AudioChannelLayout acl2 = { 0 };
        acl2.mChannelLayoutTag = kAudioChannelLayoutTag_MPEG_7_1_B;
        BaseT::setInputChannelLayout(acl2);
    }
}

std::shared_ptr<AudioChannelLayout> AudioConverterXX::getOutputChannelLayout()
{
    if (!isOutputAAC() || m_OutputChannelLayoutTag != AAC_7_1_Rear_Tag)
        return BaseT::getOutputChannelLayout();
    else {
        auto acl =
            std::shared_ptr<AudioChannelLayout>(new AudioChannelLayout);
        memset(acl.get(), 0, sizeof(AudioChannelLayout));
        acl->mChannelLayoutTag = m_OutputChannelLayoutTag;
        return acl;
    }
}

void AudioConverterXX::setOutputChannelLayout(const AudioChannelLayout &acl)
{
    if (!isOutputAAC() || acl.mChannelLayoutTag != AAC_7_1_Rear_Tag)
        BaseT::setOutputChannelLayout(acl);
    else {
        m_OutputChannelLayoutTag = acl.mChannelLayoutTag;
        AudioChannelLayout acl2 = { 0 };
        acl2.mChannelLayoutTag = kAudioChannelLayoutTag_MPEG_7_1_B;
        BaseT::setOutputChannelLayout(acl2);
    }
}

bool AudioConverterXX::isOutputAAC()
{
    auto asbd = getOutputStreamDescription();
    return (asbd.mFormatID == 'aac ' || asbd.mFormatID == 'aach');
}

double AudioConverterXX::getClosestAvailableBitRate(double value)
{
    auto rates = getApplicableEncodeBitRates();
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

void AudioConverterXX::configAACCodec(UInt32 bitrateControlMode, double bitrate,
                                      UInt32 codecQuality)
{
    const UInt32 qmax = kAudioConverterQuality_Max;

    setBitRateControlMode(bitrateControlMode);
    setCodecQuality(std::min(codecQuality, qmax));

    if (bitrateControlMode == kAudioCodecBitRateControlMode_Variable)
        setSoundQualityForVBR(std::min(UInt32(bitrate + .5), qmax));
    else {
        if (bitrate == 0.0) // request maximum available bitrate
            bitrate = 1000.0 * 1000.0 * 1000.0; // set big enough value
        else if (bitrate >= 8000) // in bps
            ;
        else if (bitrate >= 8.0) // in kbps
            bitrate *= 1000.0;
        else { // bits per sample
            auto iasbd = getInputStreamDescription();
            auto oasbd = getOutputStreamDescription();
            auto layout = getOutputChannelLayout();
            double srate = oasbd.mSampleRate ? oasbd.mSampleRate
                                             : iasbd.mSampleRate;
            unsigned nchannels = oasbd.mChannelsPerFrame;
            switch (layout->mChannelLayoutTag) {
            case kAudioChannelLayoutTag_MPEG_5_1_D:
            case kAudioChannelLayoutTag_AAC_6_1:
            case kAudioChannelLayoutTag_MPEG_7_1_B:
                --nchannels;
            }
            bitrate *= nchannels * srate;
        }
        setEncodeBitRate(getClosestAvailableBitRate(bitrate));
    }
}

std::string AudioConverterXX::getConfigAsString()
{
    std::string s;
    auto asbd = getOutputStreamDescription();
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

std::string AudioConverterXX::getEncodingParamsTag()
{
    UInt32 mode = getBitRateControlMode();
    UInt32 bitrate = getEncodeBitRate();
    auto asbd = getOutputStreamDescription();
    UInt32 codec = asbd.mFormatID;
    AudioComponentDescription cd = { 'aenc', codec, 'appl', 0, 0 };
    auto ac = AudioComponentFindNext(nullptr, &cd);
    UInt32 vers = 0;
    AudioComponentGetVersion(ac, &vers);

    char buf[32] = "vers\0\0\0\1acbf\0\0\0\0brat\0\0\0\0cdcv\0\0\0";
    buf[12] = mode >> 24;
    buf[13] = mode >> 16;
    buf[14] = mode >> 8;
    buf[15] = mode;
    buf[20] = bitrate >> 24;
    buf[21] = bitrate >> 16;
    buf[22] = bitrate >> 8;
    buf[23] = bitrate;
    buf[28] = vers >> 24;
    buf[29] = vers >> 16;
    buf[30] = vers >> 8;
    buf[31] = vers;
    return std::string(buf, buf + 32);
}
