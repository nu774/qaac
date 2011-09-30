#ifndef _AACENCODER_H
#define _AACENCODER_H

#include "encoderbase.h"

class AACEncoder : public EncoderBase {
public:
    AACEncoder(const x::shared_ptr<ISource> &src, uint32_t formatID,
	    int nchannelsOut, int chanmask = -1)
	: EncoderBase(src, formatID, nchannelsOut, chanmask)
    {}
    void getGaplessInfo(GaplessInfo *info) const;
    void setEncoderParameter(const wchar_t *key, int value);
    int getParameterRange(const wchar_t *key,
	    CFArrayT<CFStringRef> *avails, CFArrayT<CFStringRef> *limits=0);
    void forceAACChannelMapping();
};

#endif
