#ifndef AudioConverterX_H
#define AudioConverterX_H

#include <stdint.h>
#include <memory>
#include <string>
#include <vector>
#include "CoreAudio/AudioConverter.h"

class AudioConverterX {
    std::shared_ptr<OpaqueAudioConverter> m_converter;
public:
    AudioConverterX() {}
    virtual ~AudioConverterX() {}
    AudioConverterX(AudioConverterRef converter, bool takeOwn)
    {
        attach(converter, takeOwn);
    }
    AudioConverterX(const AudioStreamBasicDescription &iasbd,
                    const AudioStreamBasicDescription &oasbd);
    operator AudioConverterRef() { return m_converter.get(); }
    void attach(AudioConverterRef converter, bool takeOwn);

    // properties
    UInt32 getSampleRateConverterComplexity();
    void setSampleRateConverterComplexity(UInt32 complexity);
    UInt32 getSampleRateConverterQuality();
    void setSampleRateConverterQuality(UInt32 quality);
    UInt32 getPrimeMethod();
    void setPrimeMethod(UInt32 value);
    void getPrimeInfo(AudioConverterPrimeInfo *result);
    void setPrimeInfo(const AudioConverterPrimeInfo &info);
    virtual void getCompressionMagicCookie(std::vector<uint8_t> *result);
    void setDecompressionMagicCookie(const std::vector<uint8_t> &cookie);
    UInt32 getEncodeBitRate();
    void setEncodeBitRate(UInt32 value);
    virtual
    void getInputChannelLayout(std::shared_ptr<AudioChannelLayout> *result);
    virtual void setInputChannelLayout(const AudioChannelLayout &value);
    virtual
    void getOutputChannelLayout(std::shared_ptr<AudioChannelLayout> *result);
    virtual void setOutputChannelLayout(const AudioChannelLayout &value);
    void getApplicableEncodeBitRates(std::vector<AudioValueRange> *result);
    void getApplicableEncodeSampleRates(std::vector<AudioValueRange> *result);
    void getInputStreamDescription(AudioStreamBasicDescription *result);
    void getOutputStreamDescription(AudioStreamBasicDescription *result);
    UInt32 getMaximumOutputPacketSize();
    UInt32 getBitRateControlMode();
    void setBitRateControlMode(UInt32 value);
    UInt32 getSoundQualityForVBR();
    void setSoundQualityForVBR(UInt32 value);
    UInt32 getCodecQuality();
    void setCodecQuality(UInt32 value);
};

#endif
