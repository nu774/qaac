#ifndef _AACENCODER_H
#define _AACENCODER_H

#include "encoderbase.h"
#include "aacconfig.h"

class AACEncoder : public EncoderBase {
public:
    AACEncoder(const x::shared_ptr<ISource> &src, uint32_t formatID,
	    int nchannelsOut, int chanmask = -1)
	: EncoderBase(src, formatID, nchannelsOut, chanmask)
    {}
    void getCodecConfigArray(CFArrayT<CFDictionaryRef> *result);
    void setParameters(const std::vector<aac::Config> &params);
    void getGaplessInfo(GaplessInfo *info) const;
    void forceAACChannelMapping();
};

#endif
