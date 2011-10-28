#ifndef AACCONFIG_H
#define AACCONFIG_H

#include "stdaudio.h"
#include "util.h"

struct CodecSetting {
    uint32_t m_codec;
    uint32_t m_sample_rate;
    uint32_t m_nchannels;
    std::wstring m_channel_layout;
    std::vector<uint32_t> m_bitrates;
};


namespace aac {

enum ParamType {
    kStrategy, kBitRate, kTVBRQuality, kQuality
};

struct Config {
    ParamType type;
    int value;
};

const wchar_t *GetParamName(ParamType param);
CFArrayRef GetParametersFromSettings(CFArrayRef settings);
void GetCodecConfigArray(StdAudioComponentX *encoder,
			 CFArrayT<CFDictionaryRef> *result);
void SetParameters(StdAudioComponentX *encoder,
		   const std::vector<Config> &params);
int GetParameterRange(CFArrayRef parameters, ParamType param,
	CFArrayT<CFStringRef> *available, CFArrayT<CFStringRef> *limits=0);
int GetParameterRange(StdAudioComponentX *encoder, ParamType param,
	CFArrayT<CFStringRef> *available, CFArrayT<CFStringRef> *limits=0);
void GetAvailableSettings(std::vector<CodecSetting> *settings);

}

#endif
