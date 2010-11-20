#include "aacencoder.h"

static
CFArrayRef GetParametersFromSettings(CFArrayRef settings)
{
    CFDictionaryRef dict
	= SearchCFDictArray(settings,
		CFSTR("converter"), CFSTR("CodecConverter"));
    if (!dict)
	throw std::runtime_error("GetPrametersFromSettings: "
		"CodecConverter not found");
    return CFDictionaryGetValueT<CFArrayRef>(dict, CFSTR("parameters"));
}

static
CFDictionaryRef GetParameterDictFromSettings(CFArrayRef settings,
	const wchar_t *key)
{
    CFArrayRef parameters = GetParametersFromSettings(settings);
    CFDictionaryRef dict =
	SearchCFDictArray(parameters, CFSTR("key"), W2CF(key));
    if (!dict)
	throw std::runtime_error(format("Key not found: %ls", key));
    return dict;
}

void AACEncoder::setEncoderParameter(const wchar_t *key, int value)
{
    CFArrayT<CFDictionaryRef> settings;
    getCodecSpecificSettingsArray(&settings);
    CFDictionaryRef dict = GetParameterDictFromSettings(settings, key);
    CFStringRef value_key = CFSTR("current value");
    if (!std::wcscmp(key, L"Bit Rate")
	    && CFDictionaryGetValue(dict, CFSTR("slider value")))
	value_key = CFSTR("slider value");
    CFDictionaryReplaceValue(const_cast<CFMutableDictionaryRef>(dict),
	    value_key, CFNumberCreateT(value));
    setCodecSpecificSettingsArray(settings);
    getBasicDescription(&m_output_desc);
}

int AACEncoder::getParameterRange(const wchar_t *key,
	CFArrayT<CFStringRef> *result, CFArrayT<CFStringRef> *limits)
{
    int n;
    CFArrayT<CFDictionaryRef> settings;
    getCodecSpecificSettingsArray(&settings);
    CFDictionaryRef dict = GetParameterDictFromSettings(settings, key);
    if (!std::wcscmp(key, L"Bit Rate")
	    && CFDictionaryGetValue(dict, CFSTR("slider value"))) {
	CFNumberRef v = CFDictionaryGetValueT<CFNumberRef>(dict, CFSTR("max"));
	CFNumberGetValue(v, kCFNumberSInt32Type, &n);
	*result = CFArrayT<CFStringRef>();
	return n;
    } else {
	CFArrayRef v = CFDictionaryGetValueT<CFArrayRef>(
			dict, CFSTR("available values"));
	*result = CFArrayT<CFStringRef>(CFArrayCreateCopy(0, v));
	if (limits) {
	    v = CFDictionaryGetValueT<CFArrayRef>(
		    dict, CFSTR("limited values"));
	    *limits = CFArrayT<CFStringRef>(CFArrayCreateCopy(0, v));
	}
	return result->size();
    }
}

void AACEncoder::getGaplessInfo(GaplessInfo *info) const
{
    info->delay = 0x840;
    info->samples = m_samples_read;
    if (m_input_desc.mSampleRate != m_output_desc.mSampleRate)
	info->samples = static_cast<uint64_t>(m_samples_read *
	    (m_output_desc.mSampleRate / m_input_desc.mSampleRate));
    uint32_t frame_length = m_output_desc.mFramesPerPacket;
    if (m_output_desc.mFormatID == 'aach') {
	info->samples /= 2;
	frame_length /= 2;
    }
    info->padding = static_cast<uint32_t>(
	    m_frames_written * frame_length - info->delay - info->samples);
}
