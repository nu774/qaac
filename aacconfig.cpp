#include "aacconfig.h"

struct AACConfigKey {
    const wchar_t *name;
    const wchar_t *valueKey;
};

static const AACConfigKey AACConfigKeyTable[] = {
    { L"Target Format", L"current value" },
    { L"Bit Rate", L"current value" },
    { L"Bit Rate", L"slider value" },
    { L"Quality", L"current value" },
    { 0, 0 }
};

static
void GetCopyOfCodecSpecificSettingsArray(StdAudioComponentX *encoder,
	CFArrayT<CFDictionaryRef> *settings)
{
    CFArrayT<CFDictionaryRef> array;
    encoder->getCodecSpecificSettingsArray(&array);
    CFMutableArrayRef copy = (CFMutableArrayRef)(CloneCFObject(array));
    CFArrayT<CFDictionaryRef> newSettings(copy);
    settings->swap(newSettings);
}

static
CFDictionaryRef GetParameterDict(CFArrayRef parameters, const wchar_t *key)
{
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
	    aac::GetParameterRange(&scaudio,
		    aac::kBitRate, &available, &limits);
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

extern void dump_object(CFTypeRef ref, std::ostream &os);

namespace aac {

const wchar_t *GetParamName(ParamType param)
{
    return AACConfigKeyTable[param].name;
}

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

void SetParameters(StdAudioComponentX *encoder,
		   const std::vector<Config> &params)
{
    CFArrayT<CFDictionaryRef> settings;
    GetCopyOfCodecSpecificSettingsArray(encoder, &settings);
    CFArrayRef parameters = GetParametersFromSettings(settings);

    std::vector<Config>::const_iterator it;
    for (it = params.begin(); it != params.end(); ++it) {
	const AACConfigKey *ent = &AACConfigKeyTable[it->type];
	CFMutableDictionaryRef dict = const_cast<CFMutableDictionaryRef>(
		    GetParameterDict(parameters, ent->name));
	CFDictionarySetValue(dict, W2CF(ent->valueKey),
			     CFNumberCreateT(it->value));
    }
//    dump_object(settings, std::cerr);
    encoder->setCodecSpecificSettingsArray(settings);
}

int GetParameterRange(CFArrayRef parameters, ParamType param,
	CFArrayT<CFStringRef> *result, CFArrayT<CFStringRef> *limits)
{
    const AACConfigKey *ent = &AACConfigKeyTable[param];
    CFDictionaryRef dict = GetParameterDict(parameters, ent->name);
    if (param != kTVBRQuality) {
	CFArrayRef v = CFDictionaryGetValueT<CFArrayRef>(
			dict, CFSTR("available values"));
	CFArrayT<CFStringRef> avail(static_cast<CFArrayRef>(CFRetain(v)));
	result->swap(avail);
	if (limits) {
	    v = CFDictionaryGetValueT<CFArrayRef>(
		    dict, CFSTR("limited values"));
	    CFArrayT<CFStringRef> limited(static_cast<CFArrayRef>(CFRetain(v)));
	    limits->swap(limited);
	}
    }
    CFNumberRef nr =
	CFDictionaryGetValueT<CFNumberRef>(dict, W2CF(ent->valueKey));
    int n;
    CFNumberGetValue(nr, kCFNumberSInt32Type, &n);
    return n;
}

int GetParameterRange(StdAudioComponentX *encoder, ParamType param,
	CFArrayT<CFStringRef> *result, CFArrayT<CFStringRef> *limits)
{
    CFArrayT<CFDictionaryRef> settings;
    encoder->getCodecSpecificSettingsArray(&settings);
    CFArrayRef parameters = GetParametersFromSettings(settings);
    return GetParameterRange(parameters, param, result, limits);
}

void GetAvailableSettings(std::vector<CodecSetting> *settings)
{
    std::vector<CodecSetting> v;
    GetAvailableSettingsForCodec('aac ', &v);
    GetAvailableSettingsForCodec('aach', &v);
    if (settings) settings->swap(v);
}

} // namespace aac
