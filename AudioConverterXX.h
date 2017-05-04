#ifndef AudioConverterXX_H
#define AudioConverterXX_H

#include "AudioConverterX.h"

class AudioConverterXX : public AudioConverterX {
public:
    AudioConverterXX() {}
    AudioConverterXX(AudioConverterRef converter, bool takeOwn)
        : AudioConverterX(converter, takeOwn)
    {}
    AudioConverterXX(const AudioStreamBasicDescription &iasbd,
                     const AudioStreamBasicDescription &oasbd)
        : AudioConverterX(iasbd, oasbd)
    {}
    double getClosestAvailableBitRate(double value);
    std::string getConfigAsString();
    std::string getEncodingParamsTag();
};

#endif
