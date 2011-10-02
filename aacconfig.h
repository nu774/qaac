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

void SetParameter(StdAudioComponentX *encoder, const wchar_t *key, int value);
int GetParameterRange(StdAudioComponentX *encoder, const wchar_t *key,
	CFArrayT<CFStringRef> *available, CFArrayT<CFStringRef> *limits);
void GetAvailableSettings(std::vector<CodecSetting> *settings);

}

