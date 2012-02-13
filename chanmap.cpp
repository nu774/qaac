#include "chanmap.h"

namespace chanmap {

const char *GetChannelName(uint32_t n)
{
    const char *tab[] = {
	"?","L","R","C","LFE",
	"Ls","Rs","Lc","Rc","Cs",
	"Lsd","Rsd","Ts","Vhl","Vhc",
	"Vhr","Tbl","Tbc","Tbr"
    };
    if (n <= 18) return tab[n];
    switch (n) {
    case 33: return "Rls";
    case 34: return "Rrs";
    case 35: return "Lw";
    case 36: return "Rw";
    }
    return "?";
}

std::string GetChannelNames(const std::vector<uint32_t> &channels)
{
    std::string result;
    const char *delim = "";
    size_t lfe_count = 0;
    for (size_t i = 0; i < channels.size(); ++i, delim = " ") {
	result += delim;
	result += GetChannelName(channels[i]);
	if (channels[i] == 4) ++lfe_count;
    }
    size_t count = channels.size();
    count -= lfe_count;
    return format("%u.%u (%s)",
		  static_cast<uint32_t>(count),
		  static_cast<uint32_t>(lfe_count),
		  result.c_str());
}

uint32_t GetChannelMask(const std::vector<uint32_t>& channels)
{
    uint32_t result = 0;
    for (size_t i = 0; i < channels.size(); ++i) {
	if (channels[i] >= 33)
	    throw std::runtime_error("Not supported channel layout");
	result |= (1 << (channels[i] - 1));
    }
    return result;
}

void GetChannels(uint32_t bitmap, std::vector<uint32_t> *result,
		 uint32_t limit)
{
    std::vector<uint32_t> channels;
    for (unsigned i = 0; i < 32 && channels.size() < limit; ++i) {
	if (bitmap & (1<<i))
	    channels.push_back(i + 1);
    }
    result->swap(channels);
}

void GetChannels(const AudioChannelLayout *acl, std::vector<uint32_t> *result)
{
    std::vector<uint32_t> channels;
    uint32_t bitmap = 0;
    const char *layout = 0;

    switch (acl->mChannelLayoutTag) {
    case kAudioChannelLayoutTag_UseChannelBitmap:
	bitmap = acl->mChannelBitmap; break;
    case kAudioChannelLayoutTag_UseChannelDescriptions:
    {
	const AudioChannelDescription *desc = acl->mChannelDescriptions;
	for (size_t i = 0; i < acl->mNumberChannelDescriptions; ++i)
	    channels.push_back(desc[i].mChannelLabel);
	break;
    }
    case kAudioChannelLayoutTag_Mono:
	layout = "\x03"; break;
    case kAudioChannelLayoutTag_AC3_1_0_1:
	layout = "\x03\x04"; break;
    case kAudioChannelLayoutTag_Stereo:
	layout = "\x01\x02"; break;
    case kAudioChannelLayoutTag_DVD_4:
	layout = "\x01\x02\x04"; break;
    case kAudioChannelLayoutTag_MPEG_3_0_A:
	layout = "\x01\x02\x03"; break;
    case kAudioChannelLayoutTag_MPEG_3_0_B:
	layout = "\x03\x01\x02"; break;
    case kAudioChannelLayoutTag_AC3_3_0:
	layout = "\x01\x03\x02"; break;
    case kAudioChannelLayoutTag_ITU_2_1:
	layout = "\x01\x02\x09"; break;
    case kAudioChannelLayoutTag_DVD_10:
	layout = "\x01\x02\x03\x04"; break;
    case kAudioChannelLayoutTag_AC3_3_0_1:
	layout = "\x01\x03\x02\x04"; break;
    case kAudioChannelLayoutTag_DVD_5:
	layout = "\x01\x02\x04\x09"; break;
    case kAudioChannelLayoutTag_AC3_2_1_1:
	layout = "\x01\x02\x09\x04"; break;
    case kAudioChannelLayoutTag_Quadraphonic:
    case kAudioChannelLayoutTag_ITU_2_2:
	layout = "\x01\x02\x05\x06"; break;
    case kAudioChannelLayoutTag_MPEG_4_0_A:
	layout = "\x01\x02\x03\x09"; break;
    case kAudioChannelLayoutTag_MPEG_4_0_B:
	layout = "\x03\x01\x02\x09"; break;
    case kAudioChannelLayoutTag_AC3_3_1:
	layout = "\x01\x03\x02\x09"; break;
    case kAudioChannelLayoutTag_DVD_6:
	layout = "\x01\x02\x04\x05\x06"; break;
    case kAudioChannelLayoutTag_DVD_18:
	layout = "\x01\x02\x05\x06\x04"; break;
    case kAudioChannelLayoutTag_DVD_11:
	layout = "\x01\x02\x03\x04\x09"; break;
    case kAudioChannelLayoutTag_AC3_3_1_1:
	layout = "\x01\x03\x02\x09\x04"; break;
    case kAudioChannelLayoutTag_MPEG_5_0_A:
	layout = "\x01\x02\x03\x05\x06"; break;
    case kAudioChannelLayoutTag_MPEG_5_0_B:
	layout = "\x01\x02\x05\x06\x03"; break;
    case kAudioChannelLayoutTag_MPEG_5_0_C:
	layout = "\x01\x03\x02\x05\x06"; break;
    case kAudioChannelLayoutTag_MPEG_5_0_D:
	layout = "\x03\x01\x02\x05\x06"; break;
    case kAudioChannelLayoutTag_MPEG_5_1_A:
	layout = "\x01\x02\x03\x04\x05\x06"; break;
    case kAudioChannelLayoutTag_MPEG_5_1_B:
	layout = "\x01\x02\x05\x06\x03\x04"; break;
    case kAudioChannelLayoutTag_MPEG_5_1_C:
	layout = "\x01\x03\x02\x05\x06\x04"; break;
    case kAudioChannelLayoutTag_MPEG_5_1_D:
	layout = "\x03\x01\x02\x05\x06\x04"; break;
    case kAudioChannelLayoutTag_Hexagonal:
    case kAudioChannelLayoutTag_AudioUnit_6_0:
	layout = "\x01\x02\x05\x06\x03\x09"; break;
    case kAudioChannelLayoutTag_AAC_6_0:
	layout = "\x03\x01\x02\x05\x06\x09"; break;
    case kAudioChannelLayoutTag_MPEG_6_1_A:
	layout = "\x01\x02\x03\x04\x05\x06\x09"; break;
    case kAudioChannelLayoutTag_AAC_6_1:
	layout = "\x03\x01\x02\x05\x06\x09\x04"; break;
    case kAudioChannelLayoutTag_AudioUnit_7_0:
	layout = "\x01\x02\x05\x06\x03\x21\x22"; break;
    case kAudioChannelLayoutTag_AudioUnit_7_0_Front:
	layout = "\x01\x02\x05\x06\x03\x07\x08"; break;
    case kAudioChannelLayoutTag_AAC_7_0:
	layout = "\x03\x01\x02\x05\x06\x21\x22"; break;
    case kAudioChannelLayoutTag_MPEG_7_1_A:
	layout = "\x01\x02\x03\x04\x05\x06\x07\x08"; break;
    case kAudioChannelLayoutTag_MPEG_7_1_B:
	layout = "\x03\x07\x08\x01\x02\x05\x06\x04"; break;
    case kAudioChannelLayoutTag_MPEG_7_1_C:
	layout = "\x01\x02\x03\x04\x05\x06\x21\x22"; break;
    case kAudioChannelLayoutTag_Emagic_Default_7_1:
	layout = "\x01\x02\x05\x06\x03\x04\x07\x08"; break;
    case kAudioChannelLayoutTag_Octagonal:
	layout = "\x01\x02\x05\x06\x03\x09\x21\x22"; break;
    case kAudioChannelLayoutTag_AAC_Octagonal:
	layout = "\x03\x01\x02\x05\x06\x21\x22\x09"; break;
    default:
	throw std::runtime_error("Unsupported channel layout");
    }

    if (bitmap)
	GetChannels(bitmap, &channels);
    else if (layout)
	while (*layout) channels.push_back(*layout++);

    result->swap(channels);
}

void ConvertChannelsFromAppleLayout(const std::vector<uint32_t> &from,
				    std::vector<uint32_t> *to)
{
    struct F {
	static bool test(uint32_t x) { return x == 33 || x == 34; }
	static uint32_t trans(uint32_t x) {
	    switch (x) {
	    case 5: return 10;
	    case 6: return 11;
	    case 33: return 5;
	    case 34: return 6;
	    }
	    return x;
	}
    };
    bool has_rear =
	std::find_if(from.begin(), from.end(), F::test) != from.end();
    std::vector<uint32_t> result(from.size());
    if (!has_rear)
	std::copy(from.begin(), from.end(), result.begin());
    else
	std::transform(from.begin(), from.end(), result.begin(), F::trans);
    to->swap(result);
}

template <typename T>
class IndexComparator {
    const T *m_data;
public:
    IndexComparator(const T *data): m_data(data) {}
    bool operator()(size_t l, size_t r) { return m_data[l-1] < m_data[r-1]; }
};

void GetChannelMappingToUSBOrder(const std::vector<uint32_t> &channels,
				 std::vector<uint32_t> *result)
{
    std::vector<uint32_t> index(channels.size());
    for (unsigned i = 0; i < channels.size(); ++i)
	index[i] = i + 1;
    std::sort(index.begin(), index.end(),
	      IndexComparator<uint32_t>(&channels[0]));
    result->swap(index);
}

uint32_t GetDefaultChannelMask(const uint32_t nchannels)
{
    static const uint32_t tab[] = {
	0x4, // FC
	0x3, // FL FR
	0x7, // FL FR FC
	0x33, // FL FR BL BR
	0x37, // FL FR FC BL BR
	0x3f, // FL FR FC LFE BL BR
	0x13f, // FL FR FC LFE BL BR BC
	0x63f // FL FR FC LFE BL BR SL SR
    };
    return tab[nchannels - 1];
}

uint32_t GetLayoutTag(uint32_t bitmap)
{
    switch (bitmap) {
    case 0x4:   return kAudioChannelLayoutTag_Mono;
    case 0x3:   return kAudioChannelLayoutTag_Stereo;
    case 0x7:   return kAudioChannelLayoutTag_MPEG_3_0_A;
    case 0x33:  return kAudioChannelLayoutTag_Quadraphonic;
    case 0x107: return kAudioChannelLayoutTag_MPEG_4_0_A;
    case 0x37:  return kAudioChannelLayoutTag_MPEG_5_0_A;
    case 0x3f:  return kAudioChannelLayoutTag_MPEG_5_1_A;
    case 0x13f: return kAudioChannelLayoutTag_MPEG_6_1_A;
    case 0xff:  return kAudioChannelLayoutTag_MPEG_7_1_A;
    }
    return kAudioChannelLayoutTag_UseChannelBitmap;
}

uint32_t GetAACLayoutTag(uint32_t bitmap)
{
    if ((bitmap & 0x600) == 0x600 && (bitmap & 0x30) == 0) {
	bitmap &= ~0x600;
	bitmap |= 0x30;
    }
    switch (bitmap) {
    case 0x4:   return kAudioChannelLayoutTag_Mono;
    case 0x3:   return kAudioChannelLayoutTag_Stereo;
    case 0x7:   return kAudioChannelLayoutTag_AAC_3_0;
    case 0x33:  return kAudioChannelLayoutTag_Quadraphonic;
    case 0x107: return kAudioChannelLayoutTag_AAC_4_0;
    case 0x1c4: return kAudioChannelLayoutTag_AAC_4_0;
    case 0x37:  return kAudioChannelLayoutTag_AAC_5_0;
    case 0x3f:  return kAudioChannelLayoutTag_AAC_5_1;
    case 0x137: return kAudioChannelLayoutTag_AAC_6_0;
    case 0x13f: return kAudioChannelLayoutTag_AAC_6_1;
    case 0x637: return kAudioChannelLayoutTag_AAC_7_0;
    case 0xff:  return kAudioChannelLayoutTag_AAC_7_1;
    case 0x63f: return kAudioChannelLayoutTag_AAC_7_1; /* XXX */
    case 0x737: return kAudioChannelLayoutTag_AAC_Octagonal;
    }
    throw std::runtime_error("No channel mapping to AAC defined");
}

static
void NormalizeChannelsForAAC(uint32_t bitmap, std::vector<uint32_t> &channels)
{
    bool side = (bitmap & 0x600) == 0x600;
    bool front = (bitmap & 0x3) == 0x3;

    for (std::vector<uint32_t>::iterator
	 it = channels.begin(); it != channels.end(); ++it) {
	if (bitmap == 0x63f) // FL FR FC LFE BL BR SL SR
	    switch (*it) {
	    case 1: case 2:   *it += 6; break; /* FL, FR -> Lc, Rc */
	    case 10: case 11: *it -= 9; break; /* SL, SR -> L, R   */
	    }
	else
	    switch (*it) {
	    case 5: case 6:
		if (side)   *it += 28; break; /* BL, BR -> Rls, Rrs */
	    case 7: case 8:
		if (!front) *it -= 6;  break; /* FLC, FRC -> L, R */
	    case 10: case 11: *it -= 5; break; /* SL, SR -> Ls, Rs */
	    }
    }
}

void GetAACChannelMap(uint32_t bitmap, std::vector<uint32_t> *result)
{
    AudioChannelLayout aacLayout = { 0 };
    aacLayout.mChannelLayoutTag = GetAACLayoutTag(bitmap);

    std::vector<uint32_t> wcs, acs, mapping;
    GetChannels(bitmap, &wcs);
    GetChannels(&aacLayout, &acs);
    NormalizeChannelsForAAC(bitmap, wcs);

    if (wcs == acs)
	return;
    for (size_t i = 0; i < acs.size(); ++i) {
	std::vector<uint32_t>::iterator
	    pos = std::find(wcs.begin(), wcs.end(), acs[i]);
	if (pos == wcs.end())
	    throw std::runtime_error("No channel mapping to AAC defined");
	mapping.push_back(std::distance(wcs.begin(), pos) + 1);
    }
    result->swap(mapping);
}

} // namespace

ChannelMapper::ChannelMapper(const x::shared_ptr<ISource> &source,
			     const std::vector<uint32_t> &chanmap,
			     uint32_t bitmap)
    : DelegatingSource(source)
{
    for (size_t i = 0; i < chanmap.size(); ++i)
	m_chanmap.push_back(chanmap[i] - 1);
    if (bitmap) {
	for (unsigned i = 0; i < 32; ++i, bitmap >>= 1)
	    if (bitmap & 1) m_layout.push_back(i + 1);
    } else {
	const std::vector<uint32_t> *orig =
	    DelegatingSource::getChannels();
	if (orig)
	    for (size_t i = 0; i < m_chanmap.size(); ++i)
		m_layout.push_back(orig->at(m_chanmap[i]));
    }
}

size_t ChannelMapper::readSamples(void *buffer, size_t nsamples)
{
    const SampleFormat &sfmt = source()->getSampleFormat();
    size_t width = sfmt.m_bitsPerSample >> 3;
    size_t framelen = sfmt.bytesPerFrame();
    std::vector<char> tmp_buffer(framelen);
    size_t rc = source()->readSamples(buffer, nsamples);
    char *bp = reinterpret_cast<char*>(buffer);
    for (size_t i = 0; i < rc ; ++i, bp += framelen) {
	std::memcpy(&tmp_buffer[0], bp, framelen);
	for (size_t j = 0; j < m_chanmap.size(); ++j) {
	    std::memcpy(bp + width * j,
		    &tmp_buffer[0] + width * m_chanmap[j], width);
	}
    }
    return rc;
}
