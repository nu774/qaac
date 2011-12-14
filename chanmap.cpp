#include "chanmap.h"

uint32_t GetBitmapFromAudioChannelLayout(const AudioChannelLayout *acl)
{
    if (acl->mChannelBitmap)
	return acl->mChannelBitmap;
    else if (acl->mNumberChannelDescriptions > 0) {
	const AudioChannelDescription *desc = acl->mChannelDescriptions;
	uint32_t bitmap = 0;
	for (size_t i = 0; i < acl->mNumberChannelDescriptions - 1; ++i)
	    if (desc[i].mChannelLabel >= desc[i+1].mChannelLabel)
		return 0;
	for (size_t i = 0; i < acl->mNumberChannelDescriptions; ++i) {
	    if (desc[i].mChannelLabel >= 33)
		return 0;
	    bitmap |= (1 << (desc[i].mChannelLabel - 1));
	}
	return bitmap;
    } else {
	switch (acl->mChannelLayoutTag) {
	case kAudioChannelLayoutTag_Mono: return 0x4;
	case kAudioChannelLayoutTag_Stereo: return 0x3;
	case kAudioChannelLayoutTag_MPEG_3_0_A: return 0x7;
	case kAudioChannelLayoutTag_Quadraphonic:
	case kAudioChannelLayoutTag_ITU_2_2:
	    return 0x33;
	case kAudioChannelLayoutTag_MPEG_4_0_A: return 0x107;
	case kAudioChannelLayoutTag_MPEG_5_0_A: return 0x37;
	case kAudioChannelLayoutTag_MPEG_5_1_A: return 0x3f;
	case kAudioChannelLayoutTag_MPEG_6_1_A: return 0x13f;
	case kAudioChannelLayoutTag_MPEG_7_1_A: return 0xff;
	}
    }
    return 0;
}

/*
 * Fill in AudioChannelDescription with channel labels.
 * It seems QT looks at these labels when remix is done,
 * therefore we use this for remixing to be properly done.
 */
void MapChannelLabel(AudioChannelDescription *desc, uint32_t bitmap)
{
    std::vector<uint32_t> labels;
    bool back_in_use = ((bitmap & 0x30) == 0x30);
    bool side_in_use = ((bitmap & 0x600) == 0x600);

    for (size_t n = 0, t = bitmap; n < 32; ++n, t >>= 1)
	if (t & 1) labels.push_back(n + 1);
    for (size_t i = 0; i < labels.size(); ++i) {
	if (labels[i] == kAudioChannelLabel_LeftSurroundDirect)
	    desc[i].mChannelLabel = kAudioChannelLabel_LeftSurround;
	else if (labels[i] == kAudioChannelLabel_RightSurroundDirect)
	    desc[i].mChannelLabel = kAudioChannelLabel_RightSurround;
	else if (side_in_use && labels[i] == kAudioChannelLabel_LeftSurround)
	    desc[i].mChannelLabel = kAudioChannelLabel_RearSurroundLeft;
	else if (side_in_use && labels[i] == kAudioChannelLabel_RightSurround)
	    desc[i].mChannelLabel = kAudioChannelLabel_RearSurroundRight;
	else
	    desc[i].mChannelLabel = labels[i];
    }
}

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

uint32_t GetALACLayoutTag(uint32_t nchannels)
{
    static const uint32_t tab[] = {
	kAudioChannelLayoutTag_Mono,
	kAudioChannelLayoutTag_Stereo,
	kAudioChannelLayoutTag_AAC_3_0,
	kAudioChannelLayoutTag_AAC_4_0,
	kAudioChannelLayoutTag_AAC_5_0,
	kAudioChannelLayoutTag_AAC_5_1,
	kAudioChannelLayoutTag_AAC_6_1,
	kAudioChannelLayoutTag_AAC_7_1
    };
    return tab[nchannels-1];
}

uint32_t GetAACReversedChannelMap(uint32_t layoutTag,
	std::vector<uint32_t> *result)
{
    static const uint32_t a30[] = { 2, 3, 1, 0 };
    static const uint32_t a40[] = { 2, 3, 1, 4, 0 };
    static const uint32_t a50[] = { 2, 3, 1, 4, 5, 0 };
    static const uint32_t a51[] = { 2, 3, 1, 6, 4, 5, 0 };
    static const uint32_t a60[] = { 2, 3, 1, 4, 5, 6, 0 };
    static const uint32_t a61[] = { 2, 3, 1, 7, 4, 5, 6, 0 };
    static const uint32_t a70[] = { 2, 3, 1, 6, 7, 4, 5, 0 };
    static const uint32_t a71[] = { 4, 5, 1, 8, 6, 7, 2, 3, 0 };
    static const uint32_t a80[] = { 2, 3, 1, 6, 7, 8, 4, 5, 0 };

    const uint32_t *a = 0;
    uint32_t bitmap = 0;

    switch (layoutTag) {
    case kAudioChannelLayoutTag_Mono:
	bitmap = 0x4; break;
    case kAudioChannelLayoutTag_Stereo:
	bitmap = 0x3; break;
    case kAudioChannelLayoutTag_AAC_3_0:
	bitmap = 0x7; a = a30; break;
    case kAudioChannelLayoutTag_Quadraphonic:
	bitmap = 0x33; break;
    case kAudioChannelLayoutTag_AAC_4_0:
	bitmap = 0x107; a = a40; break;
    case kAudioChannelLayoutTag_AAC_5_0:
	bitmap = 0x37; a = a50; break;
    case kAudioChannelLayoutTag_AAC_5_1:
	bitmap = 0x3f; a = a51; break;
    case kAudioChannelLayoutTag_AAC_6_0:
	bitmap = 0x137; a = a60; break;
    case kAudioChannelLayoutTag_AAC_6_1:
	bitmap = 0x13f; a = a61; break;
    case kAudioChannelLayoutTag_AAC_7_0:
	bitmap = 0x637; a = a70; break;
    case kAudioChannelLayoutTag_AAC_7_1:
	bitmap = 0xff; a = a71; break;
    case kAudioChannelLayoutTag_AAC_Octagonal:
	bitmap = 0x737; a = a80; break;
    default:
	throw std::runtime_error("GetAACReversedLayoutTag: unknown AAC layout");
    }
    std::vector<uint32_t > vec;
    while (a && *a) vec.push_back(*a++);
    if (result) result->swap(vec);
    return bitmap;
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
