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
    CFMutableArrayRef newSettings =
	(CFMutableArrayRef)(CloneCFObject(settings));
    boost::shared_ptr<__CFArray> newSettings__(newSettings, CFRelease);

    CFDictionaryRef dict = GetParameterDictFromSettings(newSettings, key);
    CFStringRef value_key = CFSTR("current value");
    if (!std::wcscmp(key, L"Bit Rate")
	    && CFDictionaryGetValue(dict, CFSTR("slider value")))
	value_key = CFSTR("slider value");
    CFDictionaryReplaceValue(const_cast<CFMutableDictionaryRef>(dict),
	    value_key, CFNumberCreateT(value));
    setCodecSpecificSettingsArray(newSettings);
    getBasicDescription(&m_output_desc);
}

int AACEncoder::getParameterRange(const wchar_t *key,
	CFArrayT<CFStringRef> *result, CFArrayT<CFStringRef> *limits)
{
    int n;
    CFArrayT<CFDictionaryRef> settings;
    getCodecSpecificSettingsArray(&settings);
    CFDictionaryRef dict = GetParameterDictFromSettings(settings, key);
    if (!std::wcscmp(key, L"Bit Rate")) {
	try {
	    CFNumberRef v = CFDictionaryGetValueT<CFNumberRef>(
		dict, CFSTR("slider value"));
	    *result = CFArrayT<CFStringRef>();
	    CFNumberGetValue(v, kCFNumberSInt32Type, &n);
	    return n;
	} catch (const std::exception &) {}
    } 
    CFArrayRef v = CFDictionaryGetValueT<CFArrayRef>(
		    dict, CFSTR("available values"));
    *result = CFArrayT<CFStringRef>((CFArrayRef)CFRetain(v));
    if (limits) {
	v = CFDictionaryGetValueT<CFArrayRef>(
		dict, CFSTR("limited values"));
	*limits = CFArrayT<CFStringRef>((CFArrayRef)CFRetain(v));
    }
    CFNumberRef nr =
	CFDictionaryGetValueT<CFNumberRef>(dict, CFSTR("current value"));
    CFNumberGetValue(nr, kCFNumberSInt32Type, &n);
    return n;
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

void AACEncoder::forceAACChannelMapping()
{
    AudioChannelLayoutX layout;
    getInputChannelLayout(&layout);
    std::vector<uint32_t> chanmap;
    uint32_t newtag = 
	GetAACChannelMapFromLayoutTag(layout->mChannelLayoutTag, &chanmap);
    if (!newtag) return;
    layout->mChannelLayoutTag = newtag;
    setInputChannelLayout(layout);
    boost::shared_ptr<ISource> newsrc(new ChannelMapper(m_src, chanmap));
    m_src = newsrc;
}
