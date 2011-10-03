#include "aacconfig.h"

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

static
void GetAvailableSettingsForCodec(uint32_t format,
	std::vector<CodecSetting> *settings)
{
    StdAudioComponentX scaudio;
    AudioStreamBasicDescription asbd = { 0 };
    asbd.mFormatID = format;
    asbd.mChannelsPerFrame = 2;
    scaudio.setBasicDescription(asbd);

    std::vector<AudioValueRange> rates;
    scaudio.getAvailableSampleRateList(&rates);

    std::vector<UInt32> layouts;
    scaudio.getAvailableChannelLayoutTagList(&layouts);

    CFArrayT<CFStringRef> names;
    scaudio.getAvailableChannelLayoutTagNamesList(&names);

    for (size_t i = 0; i < rates.size(); ++i) {
	for (size_t j = 0; j < layouts.size(); ++j) {
	    uint32_t nchannels =
		AudioChannelLayoutTag_GetNumberOfChannels(layouts[j]);
	    asbd.mSampleRate = rates[i].mMinimum;
	    asbd.mChannelsPerFrame = nchannels;
	    scaudio.setBasicDescription(asbd);
	    AudioChannelLayoutX layout;
	    layout->mChannelLayoutTag = layouts[j];
	    scaudio.setChannelLayout(layout);

	    CFArrayT<CFStringRef> available, limits;
	    aac::GetParameterRange(&scaudio, L"Bit Rate", &available, &limits);
	    CodecSetting setting;
	    setting.m_codec = format;
	    setting.m_sample_rate = asbd.mSampleRate;
	    setting.m_nchannels = nchannels;
	    setting.m_channel_layout = CF2W(names.at(j));
	    for (size_t m = 0; m < limits.size(); ++m) {
		int bits;
		std::wstring rate = CF2W(limits.at(m));
		if (swscanf(rate.c_str(), L"%d", &bits) == 1)
		    setting.m_bitrates.push_back(bits);
	    }
	    settings->push_back(setting);
	}
    }
}

namespace aac {

void SetParameter(StdAudioComponentX *encoder, const wchar_t *key, int value)
{
    CFArrayT<CFDictionaryRef> settings;
    encoder->getCodecSpecificSettingsArray(&settings);
    CFMutableArrayRef newSettings =
	(CFMutableArrayRef)(CloneCFObject(settings));
    x::shared_ptr<__CFArray> newSettings__(newSettings, CFRelease);

    CFDictionaryRef dict = GetParameterDictFromSettings(newSettings, key);
    CFStringRef value_key = CFSTR("current value");
    if (!std::wcscmp(key, L"Bit Rate")
	    && CFDictionaryGetValue(dict, CFSTR("slider value")))
	value_key = CFSTR("slider value");
    CFDictionaryReplaceValue(const_cast<CFMutableDictionaryRef>(dict),
	    value_key, CFNumberCreateT(value));
    encoder->setCodecSpecificSettingsArray(newSettings);
}

int GetParameterRange(StdAudioComponentX *encoder, const wchar_t *key,
	CFArrayT<CFStringRef> *result, CFArrayT<CFStringRef> *limits)
{
    int n;
    CFArrayT<CFDictionaryRef> settings;
    encoder->getCodecSpecificSettingsArray(&settings);
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

void GetAvailableSettings(std::vector<CodecSetting> *settings)
{
    std::vector<CodecSetting> v;
    GetAvailableSettingsForCodec('aac ', &v);
    GetAvailableSettingsForCodec('aach', &v);
    if (settings) settings->swap(v);
}

} // namespace aac
