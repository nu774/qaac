#ifndef AudioConverterXX_H
#define AudioConverterXX_H

#include "AudioConverterX.h"

class AudioConverterXX : public AudioConverterX {
private:
    typedef AudioConverterX BaseT;

    UInt32 m_InputChannelLayoutTag;
    UInt32 m_OutputChannelLayoutTag;
public:
    AudioConverterXX()
        : m_InputChannelLayoutTag(0),
          m_OutputChannelLayoutTag(0)
    {}
    AudioConverterXX(AudioConverterRef converter, bool takeOwn)
        : BaseT(converter, takeOwn),
          m_InputChannelLayoutTag(0),
          m_OutputChannelLayoutTag(0)
    {}
    AudioConverterXX(const AudioStreamBasicDescription &iasbd,
                     const AudioStreamBasicDescription &oasbd)
        : BaseT(iasbd, oasbd),
          m_InputChannelLayoutTag(0),
          m_OutputChannelLayoutTag(0)
    {}

    // overrides to support AAC 7.1ch rear encoding
    virtual std::vector<uint8_t> getCompressionMagicCookie();
    virtual std::shared_ptr<AudioChannelLayout> getInputChannelLayout();
    virtual void setInputChannelLayout(const AudioChannelLayout &value);
    virtual std::shared_ptr<AudioChannelLayout> getOutputChannelLayout();
    virtual void setOutputChannelLayout(const AudioChannelLayout &value);

    //utilities
    bool isOutputAAC();
    double getClosestAvailableBitRate(double value);
    void configAACCodec(UInt32 bitrateControlMode, double bitrate,
                        UInt32 codecQuality);
    std::string getConfigAsString();
    std::string getEncodingParamsTag();
};

#endif
