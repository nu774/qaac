#include "chanmap.h"

uint32_t GetChannelMask(const std::vector<uint32_t>& chanmap)
{
    /* only accept usb order */
    for (size_t i = 1; i < chanmap.size(); ++i)
	if (chanmap[i-1] >= chanmap[i])
	    throw std::runtime_error("Only USB ordered channel layout supported");

    uint32_t result = 0;
    for (size_t i = 0; i < chanmap.size(); ++i)
	result |= (1 << (chanmap[i] - 1));
    return result;
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

uint32_t GetAACChannelMap(const AudioChannelLayout *layout,
	std::vector<uint32_t> *result)
{
    static const uint32_t a30[] = { 3, 1, 2, 0 };
    static const uint32_t a40[] = { 3, 1, 2, 4, 0 };
    static const uint32_t a40pt[] = { 1, 2, 3, 4, 0 };
    static const uint32_t a50[] = { 3, 1, 2, 4, 5, 0 };
    static const uint32_t a51[] = { 3, 1, 2, 5, 6, 4, 0 };
    static const uint32_t a60[] = { 3, 1, 2, 4, 5, 6, 0 };
    static const uint32_t a61[] = { 3, 1, 2, 5, 6, 7, 4, 0 };
    static const uint32_t a61b[] = { 3, 1, 2, 6, 7, 5, 4, 0 };
    static const uint32_t a70[] = { 3, 1, 2, 6, 7, 4, 5, 0 };
    static const uint32_t a71a[] = { 3, 7, 8, 1, 2, 5, 6, 4, 0 };
    static const uint32_t a71c[] = { 3, 1, 2, 7, 8, 5, 6, 4, 0 };
    static const uint32_t a80[] = { 3, 1, 2, 7, 8, 4, 5, 6, 0 };

    const uint32_t *a = 0;
    uint32_t newtag = 0;

    switch (layout->mChannelLayoutTag) {
    case kAudioChannelLayoutTag_Mono:
    case kAudioChannelLayoutTag_Stereo:
    case kAudioChannelLayoutTag_Quadraphonic:
	/* these are pass-through, remapping not required */
	break;
    case kAudioChannelLayoutTag_MPEG_3_0_A:
	a = a30; newtag = kAudioChannelLayoutTag_AAC_3_0; break;
    case kAudioChannelLayoutTag_MPEG_4_0_A:
	a = a40; newtag = kAudioChannelLayoutTag_AAC_4_0; break;
    case kAudioChannelLayoutTag_MPEG_5_0_A:
	a = a50; newtag = kAudioChannelLayoutTag_AAC_5_0; break;
    case kAudioChannelLayoutTag_MPEG_5_1_A:
	a = a51; newtag = kAudioChannelLayoutTag_AAC_5_1; break;
    case kAudioChannelLayoutTag_MPEG_6_1_A:
	a = a61; newtag = kAudioChannelLayoutTag_AAC_6_1; break;
    case kAudioChannelLayoutTag_MPEG_7_1_A:
	a = a71a; newtag = kAudioChannelLayoutTag_AAC_7_1; break;
    case kAudioChannelLayoutTag_UseChannelBitmap:
	switch (layout->mChannelBitmap) {
	case 0x1c4: // FC FLC FRC BC
	    a = a40pt; newtag = kAudioChannelLayoutTag_AAC_4_0; break;
	case 0x603: // FL FR SL SR
	    a = a40pt; newtag = kAudioChannelLayoutTag_AAC_Quadraphonic; break;
	case 0x607: // FL FR FC SL SR
	    a = a50; newtag = kAudioChannelLayoutTag_AAC_5_0; break;
	case 0x60f: // FL FR FC LFE SL SR
	    a = a51; newtag = kAudioChannelLayoutTag_AAC_5_1; break;
	case 0x137: // FL FR FC LFE BL BR BC
	    a = a60; newtag = kAudioChannelLayoutTag_AAC_6_0; break;
	case 0x70f: // FL FR FC LFE BC SL SR
	    a = a61b; newtag = kAudioChannelLayoutTag_AAC_6_1; break;
	case 0x637: // FL FR FC BL BR BC SL SR
	    a = a70; newtag = kAudioChannelLayoutTag_AAC_7_0; break;
	case 0x63f: // FL FR FC LFE BL BR SL SR
	    a = a71c; newtag = kAudioChannelLayoutTag_AAC_7_1; break;
	case 0x737: // FL FR FC BL BR BC SL SR
	    a = a80; newtag = kAudioChannelLayoutTag_AAC_Octagonal; break;
	default:
	    throw std::runtime_error("Not supported channel layout");
	}
	break;
    default:
	throw std::runtime_error("Not supported channel layout");
    }
    while (a && *a && result) result->push_back(*a++);
    return newtag;
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
