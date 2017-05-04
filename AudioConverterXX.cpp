#include "AudioConverterXX.h"
#include <limits>
#include <cmath>
#include "CoreAudio/AudioCodec.h"
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

