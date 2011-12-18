#include "chanmap.h"

namespace chanmap {

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

void GetChannelsFromBitmap(uint32_t bitmap, std::vector<uint32_t> *result,
			   uint32_t limit)
{
    std::vector<uint32_t> channels;
    for (size_t i = 0; i < 32 && channels.size() < limit; ++i) {
	if (bitmap & (1<<i))
	    channels.push_back(i + 1);
    }
    result->swap(channels);
}

void GetChannelsFromAudioChannelLayout(const AudioChannelLayout *acl,
				       std::vector<uint32_t> *result)
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
	bitmap = 0x4; break;
    case kAudioChannelLayoutTag_AC3_1_0_1:
	layout = "\x03\x04"; break;
    case kAudioChannelLayoutTag_Stereo:
	bitmap = 0x3; break;
    case kAudioChannelLayoutTag_DVD_4:
	bitmap = 0xb; break;
    case kAudioChannelLayoutTag_MPEG_3_0_A:
	bitmap = 0x7; break;
    case kAudioChannelLayoutTag_MPEG_3_0_B:
	layout = "\x03\x01\x02"; break;
    case kAudioChannelLayoutTag_ITU_2_1:
	bitmap = 0x103; break;
    case kAudioChannelLayoutTag_AC3_3_0:
	layout = "\x01\x03\x02"; break;
    case kAudioChannelLayoutTag_DVD_5:
	bitmap = 0x10b; break;
    case kAudioChannelLayoutTag_DVD_10:
	bitmap = 0xf; break;
    case kAudioChannelLayoutTag_AC3_3_0_1:
	layout = "\x01\x03\x02\x04"; break;
    case kAudioChannelLayoutTag_AC3_2_1_1:
	layout = "\x01\x02\x09\x04"; break;
    case kAudioChannelLayoutTag_Quadraphonic:
    case kAudioChannelLayoutTag_ITU_2_2:
	bitmap = 0x33; break;
    case kAudioChannelLayoutTag_MPEG_4_0_A:
	bitmap = 0x107; break;
    case kAudioChannelLayoutTag_MPEG_4_0_B:
	layout = "\x03\x01\x02\x09"; break;
    case kAudioChannelLayoutTag_AC3_3_1:
	layout = "\x01\x03\x02\x09"; break;
    case kAudioChannelLayoutTag_DVD_6:
	bitmap = 0x3b; break;
    case kAudioChannelLayoutTag_DVD_11:
	bitmap = 0x10f; break;
    case kAudioChannelLayoutTag_AC3_3_1_1:
	layout = "\x01\x03\x02\x09\x04"; break;
    case kAudioChannelLayoutTag_DVD_18:
	layout = "\x01\x02\x05\x06\x04"; break;
    case kAudioChannelLayoutTag_MPEG_5_0_A:
	bitmap = 0x37; break;
    case kAudioChannelLayoutTag_MPEG_5_0_B:
	layout = "\x01\x02\x05\x06\x03"; break;
    case kAudioChannelLayoutTag_MPEG_5_0_C:
	layout = "\x01\x03\x02\x05\x06"; break;
    case kAudioChannelLayoutTag_MPEG_5_0_D:
	layout = "\x03\x01\x02\x05\x06"; break;
    case kAudioChannelLayoutTag_MPEG_5_1_A:
	bitmap = 0x3f; break;
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
	bitmap = 0x13f; break;
    case kAudioChannelLayoutTag_AAC_6_1:
	layout = "\x03\x01\x02\x05\x06\x09\x04"; break;
    case kAudioChannelLayoutTag_AudioUnit_7_0:
	layout = "\x01\x02\x0a\x0b\x03\x05\x06"; break;
    case kAudioChannelLayoutTag_AudioUnit_7_0_Front:
	layout = "\x01\x02\x05\x06\x03\x07\x08"; break;
    case kAudioChannelLayoutTag_AAC_7_0:
	layout = "\x03\x01\x02\x0a\x0b\x05\x06"; break;
    case kAudioChannelLayoutTag_MPEG_7_1_A:
	bitmap = 0xff; break;
    case kAudioChannelLayoutTag_MPEG_7_1_B:
	layout = "\x03\x07\x08\x01\x02\x05\x06\x04"; break;
    case kAudioChannelLayoutTag_MPEG_7_1_C:
	layout = "\x01\x02\x03\x04\x0a\x0b\x05\x06"; break;
    case kAudioChannelLayoutTag_Emagic_Default_7_1:
	layout = "\x01\x02\x05\x06\x03\x04\x07\x08"; break;
    case kAudioChannelLayoutTag_Octagonal:
	layout = "\x01\x02\x05\x06\x03\x09\x0a\x0b"; break;
    case kAudioChannelLayoutTag_AAC_Octagonal:
	layout = "\x03\x01\x02\x0a\x0b\x05\x06\x09"; break;
    default:
	throw std::runtime_error("Unsupported channel layout");
    }

    if (bitmap)
	GetChannelsFromBitmap(bitmap, &channels);
    else if (layout)
	while (*layout) channels.push_back(*layout++);

    result->swap(channels);
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
    for (size_t i = 0; i < channels.size(); ++i)
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

uint32_t GetLayoutTag(uint32_t channelMask)
{
    switch (channelMask) {
    case 0x4: // FC
	return kAudioChannelLayoutTag_Mono;
    case 0x3: // FL FR
	return kAudioChannelLayoutTag_Stereo;
    case 0x7: // FL FR FC
	return kAudioChannelLayoutTag_MPEG_3_0_A;
    case 0x33: // FL FR BL BR
	return kAudioChannelLayoutTag_Quadraphonic;
    case 0x107: // FL FR FC BC
	return kAudioChannelLayoutTag_MPEG_4_0_A;
    case 0x37: // FL FR FC BL BR
	return kAudioChannelLayoutTag_MPEG_5_0_A;
    case 0x3f: // FL FR FC LFE BL BR
	return kAudioChannelLayoutTag_MPEG_5_1_A;
    case 0x13f: // FL FR FC LFE BL BR BC
	return kAudioChannelLayoutTag_MPEG_6_1_A;
    case 0xff: // FL FR FC LFE BL BR FLC FRC
	return kAudioChannelLayoutTag_MPEG_7_1_A;
    }
    return kAudioChannelLayoutTag_UseChannelBitmap;
}

uint32_t GetAACLayoutTag(const AudioChannelLayout *layout)
{
    switch (layout->mChannelLayoutTag) {
    case kAudioChannelLayoutTag_Mono:
    case kAudioChannelLayoutTag_Stereo:
    case kAudioChannelLayoutTag_Quadraphonic:
	return layout->mChannelLayoutTag;
    case kAudioChannelLayoutTag_MPEG_3_0_A:
	return kAudioChannelLayoutTag_AAC_3_0;
    case kAudioChannelLayoutTag_MPEG_4_0_A:
	return kAudioChannelLayoutTag_AAC_4_0;
    case kAudioChannelLayoutTag_MPEG_5_0_A:
	return kAudioChannelLayoutTag_AAC_5_0;
    case kAudioChannelLayoutTag_MPEG_5_1_A:
	return kAudioChannelLayoutTag_AAC_5_1;
    case kAudioChannelLayoutTag_MPEG_6_1_A:
	return kAudioChannelLayoutTag_AAC_6_1;
    case kAudioChannelLayoutTag_MPEG_7_1_A:
	return kAudioChannelLayoutTag_AAC_7_1;
    case kAudioChannelLayoutTag_UseChannelBitmap:
	switch (layout->mChannelBitmap) {
	case 0x1c4: // FC FLC FRC BC
	    return kAudioChannelLayoutTag_AAC_4_0;
	case 0x603: // FL FR SL SR
	    return kAudioChannelLayoutTag_AAC_Quadraphonic;
	case 0x607: // FL FR FC SL SR
	    return kAudioChannelLayoutTag_AAC_5_0;
	case 0x60f: // FL FR FC LFE SL SR
	    return kAudioChannelLayoutTag_AAC_5_1;
	case 0x137: // FL FR FC BL BR BC
	    return kAudioChannelLayoutTag_AAC_6_0;
	case 0x707: // FL FR FC BC SL SR
	    return kAudioChannelLayoutTag_AAC_6_0;
	case 0x70f: // FL FR FC LFE BC SL SR
	    return kAudioChannelLayoutTag_AAC_6_1;
	case 0x637: // FL FR FC BL BR SL SR
	    return kAudioChannelLayoutTag_AAC_7_0;
	case 0x6cf: // FL FR FC LFE FLC FRC SL SR
	    return kAudioChannelLayoutTag_AAC_7_1;
	case 0x63f: // FL FR FC LFE BL BR SL SR
	    return kAudioChannelLayoutTag_AAC_7_1;
	case 0x737: // FL FR FC BL BR BC SL SR
	    return kAudioChannelLayoutTag_AAC_Octagonal;
	}
    }
    throw std::runtime_error("Not supported channel layout");
}

void GetAACChannelMap(const AudioChannelLayout *layout,
    size_t nchannels, std::vector<uint32_t> *result)
{
    static const uint32_t a30[] = { 3, 1, 2, 0 };
    static const uint32_t a40[] = { 3, 1, 2, 4, 0 };
    static const uint32_t a50[] = { 3, 1, 2, 4, 5, 0 };
    static const uint32_t a51[] = { 3, 1, 2, 5, 6, 4, 0 };
    static const uint32_t a60a[] = { 3, 1, 2, 4, 5, 6, 0 };
    static const uint32_t a60b[] = { 3, 1, 2, 5, 6, 4, 0 };
    static const uint32_t a61a[] = { 3, 1, 2, 5, 6, 7, 4, 0 };
    static const uint32_t a61b[] = { 3, 1, 2, 6, 7, 5, 4, 0 };
    static const uint32_t a70[] = { 3, 1, 2, 6, 7, 4, 5, 0 };
    static const uint32_t a71a[] = { 3, 7, 8, 1, 2, 5, 6, 4, 0 };
    static const uint32_t a71b[] = { 3, 5, 6, 1, 2, 7, 8, 4, 0 };
    static const uint32_t a71c[] = { 3, 1, 2, 7, 8, 5, 6, 4, 0 };
    static const uint32_t a80[] = { 3, 1, 2, 7, 8, 4, 5, 6, 0 };

    const uint32_t *a = 0;

    switch (layout->mChannelLayoutTag) {
    case kAudioChannelLayoutTag_Mono:
    case kAudioChannelLayoutTag_Stereo:
    case kAudioChannelLayoutTag_Quadraphonic:
	/* these are pass-through, remapping not required */
	break;
    case kAudioChannelLayoutTag_MPEG_3_0_A: a = a30; break;
    case kAudioChannelLayoutTag_MPEG_4_0_A: a = a40; break;
    case kAudioChannelLayoutTag_MPEG_5_0_A: a = a50; break;
    case kAudioChannelLayoutTag_MPEG_5_1_A: a = a51; break;
    case kAudioChannelLayoutTag_MPEG_6_1_A: a = a61a; break;
    case kAudioChannelLayoutTag_MPEG_7_1_A: a = a71a; break;
    case kAudioChannelLayoutTag_UseChannelBitmap:
	switch (layout->mChannelBitmap) {
	case 0x1c4: // FC FLC FRC BC
	case 0x603: // FL FR SL SR
	/* these are pass-through, remapping not required */
	    break;
	case 0x607: a = a50; break; // FL FR FC SL SR
	case 0x60f: a = a51; break; // FL FR FC LFE SL SR
	case 0x137: a = a60a; break; // FL FR FC BL BR BC
	case 0x707: a = a60b; break; // FL FR FC BC SL SR
	case 0x70f: a = a61b; break; // FL FR FC LFE BC SL SR
	case 0x637: a = a70; break; // FL FR FC BL BR SL SR
	case 0x6cf: a = a71b; break; // FL FR FC LFE FLC FRC SL SR
	case 0x63f: a = a71c; break; // FL FR FC LFE BL BR SL SR
	case 0x737: a = a80; break; // FL FR FC BL BR BC SL SR
	default: throw std::runtime_error("Not supported channel layout");
	}
	break;
    default: throw std::runtime_error("Not supported channel layout");
    }
    while (a && *a) result->push_back(*a++);
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
	for (size_t i = 0; i < 32; ++i, bitmap >>= 1)
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
