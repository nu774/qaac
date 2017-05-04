#include "AudioConverterXX.h"
#include <limits>
#include <cmath>
#include "CoreAudio/AudioCodec.h"
#include "CoreAudio/AudioComponent.h"
#include "cautil.h"

double AudioConverterXX::getClosestAvailableBitRate(double value)
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

std::string AudioConverterXX::getConfigAsString()
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

std::string AudioConverterXX::getEncodingParamsTag()
{
    UInt32 mode = getBitRateControlMode();
    UInt32 bitrate = getEncodeBitRate();
    AudioStreamBasicDescription asbd;
    getOutputStreamDescription(&asbd);
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
