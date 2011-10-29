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

template <typename T>
struct CloserTo {
    const T &value_;
    CloserTo(const T &value): value_(value) {}
    bool operator()(const T& lhs, const T& rhs)
    {
	return std::abs(lhs - value_) < std::abs(rhs - value_);
    }
};

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

void GetCodecConfigArray(StdAudioComponentX *encoder,
	CFArrayT<CFDictionaryRef> *result)
{
    CFArrayT<CFDictionaryRef> settings;
    encoder->getCodecSpecificSettingsArray(&settings);
    CFArrayRef aref = GetParametersFromSettings(settings);
    CFArrayT<CFDictionaryRef> array(static_cast<CFArrayRef>(CFRetain(aref)));
    result->swap(array);
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

void SetupSampleRate(StdAudioComponentX &scaudio, int targetRate,
	bool isNativeResampler)
{
    int rate = targetRate;
    bool upsample = false;
    AudioStreamBasicDescription desc;
    scaudio.getInputBasicDescription(&desc);
    if (rate > desc.mSampleRate && !isNativeResampler) {
	/*
	 * Applicable output sample rate is less than input for AAC.
	 * Therefore, in order to obtain desired rate, we must cheat
	 * input sample rate here.
	 */
	upsample = true;
	desc.mSampleRate = rate;
	scaudio.setInputBasicDescription(desc);
    }
    std::vector<AudioValueRange> rates;
    scaudio.getApplicableSampleRateList(&rates);
    std::vector<int> sample_rate_table;
    for (size_t i = 0; i < rates.size(); ++i)
	sample_rate_table.push_back(rates[i].mMinimum);

    if (rate != 0) {
	int srcrate = static_cast<int>(desc.mSampleRate);
	if (rate < 0)
	    rate = srcrate;
	std::vector<int>::const_iterator it
	    = std::min_element(sample_rate_table.begin(),
		    sample_rate_table.end(), CloserTo<int>(rate));
	rate = *it;
	if (upsample && rate != desc.mSampleRate) {
	    desc.mSampleRate = rate;
	    scaudio.setInputBasicDescription(desc);
	}
    }
    AudioStreamBasicDescription oasbd;
    scaudio.getBasicDescription(&oasbd);
    oasbd.mSampleRate = rate;
    scaudio.setBasicDescription(oasbd);
}

int GetBitrateIndex(const CFArrayT<CFStringRef> &menu,
		    const CFArrayT<CFStringRef> &limits, int rate)
{
    std::vector<int> bitrates;
    std::vector<int> availables;
    for (int i = 0; i < limits.size(); ++i) {
	std::wstring s = CF2W(limits.at(i));
	int v;
	if (std::swscanf(s.c_str(), L"%d", &v) == 1)
	    availables.push_back(v);
    }
    std::sort(availables.begin(), availables.end());

    for (int i = 0; i < menu.size(); ++i) {
	std::wstring s = CF2W(menu.at(i));
	int v = 0;
	std::swscanf(s.c_str(), L"%d", &v);
	bitrates.push_back(v);
    }
    if (rate == 0)
	rate = availables[availables.size() - 1];
    else {
	std::vector<int>::iterator it
	    = std::min_element(availables.begin(),
		    availables.end(), CloserTo<int>(rate));
	rate = *it;
    }
    std::vector<int>::iterator it
	= std::find(bitrates.begin(), bitrates.end(), rate);
    return std::distance(bitrates.begin(), it);
}

void CalcGaplessInfo(const IEncoderStat *stat, bool isSBR,
	double rateRatio, GaplessInfo *result)
{
    result->delay = 0x840;
    result->samples = stat->samplesRead() * rateRatio;
    uint64_t samples_written = stat->samplesWritten();
    if (isSBR) {
	result->samples /= 2;
	samples_written /= 2;
    }
    result->padding = samples_written - result->delay - result->samples;
}
} // namespace aac
