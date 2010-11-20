#ifndef _AACENCODER_H
#define _AACENCODER_H

#include "encoderbase.h"

class AACEncoder : public EncoderBase {
public:
    AACEncoder(ISource *src, uint32_t formatID)
	: EncoderBase(src, formatID)
    {}
    void getGaplessInfo(GaplessInfo *info) const;
    void setEncoderParameter(const wchar_t *key, int value);
    int getParameterRange(const wchar_t *key, CFArrayT<CFStringRef> *result,
	    CFArrayT<CFStringRef> *limits=0);
};

#endif
