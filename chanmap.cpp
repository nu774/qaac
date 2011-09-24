#include <CoreAudioTypes.h>
#include "chanmap.h"

uint32_t
LayoutToChannelMask(const std::vector<uint32_t>& chanmap)
{
    if (chanmap.size() == 1) return 1;
    else if (chanmap.size() == 2) return 3;

    uint32_t result = 0;
    for (size_t i = 0; i < chanmap.size(); ++i)
	result |= (1 << (chanmap[i] - 1));
    return result;
}

uint32_t
GetChannelLayoutTagFromChannelMap(const std::vector<uint32_t>& chanmap)
{
    /* only accept usb order */
    for (size_t i = 1; i < chanmap.size(); ++i)
	if (chanmap[i-1] >= chanmap[i])
	    throw std::runtime_error("Not supported channel layout");

    switch (LayoutToChannelMask(chanmap)) {
    case 0x1: case 0x4:
	return kAudioChannelLayoutTag_Mono;
    case 0x3:
	return kAudioChannelLayoutTag_Stereo;
    case 0x7:
	return kAudioChannelLayoutTag_MPEG_3_0_A;
    case 0x33: case 0x603:
	return kAudioChannelLayoutTag_Quadraphonic;
    case 0x107:
	return kAudioChannelLayoutTag_MPEG_4_0_A;
    case 0x37: case 0x607:
	return kAudioChannelLayoutTag_MPEG_5_0_A;
    case 0x3f: case 0x60f:
	return kAudioChannelLayoutTag_MPEG_5_1_A;
    case 0x137: // 6.0ch
	return kAudioChannelLayoutTag_UseChannelBitmap;
    case 0x13f:
	return kAudioChannelLayoutTag_MPEG_6_1_A;
    case 0x637: // 7.0ch
	return kAudioChannelLayoutTag_UseChannelBitmap;
    case 0xff:
	return kAudioChannelLayoutTag_MPEG_7_1_A;
    case 0x63f:
	return kAudioChannelLayoutTag_MPEG_7_1_C;
    case 0x737: // 8.0ch
	return kAudioChannelLayoutTag_UseChannelBitmap;
    }
    throw std::runtime_error("Not supported channel layout");
}

uint32_t GetAACChannelMap(const AudioChannelLayout *layout,
	std::vector<uint32_t> *result)
{
    static const uint32_t a30[] = { 3, 1, 2, 0 };
    static const uint32_t a40[] = { 3, 1, 2, 4, 0 };
    static const uint32_t a50[] = { 3, 1, 2, 4, 5, 0 };
    static const uint32_t a51[] = { 3, 1, 2, 5, 6, 4, 0 };
    static const uint32_t a60[] = { 3, 1, 2, 4, 5, 6, 0 };
    static const uint32_t a61[] = { 3, 1, 2, 5, 6, 7, 4, 0 };
    static const uint32_t a70[] = { 3, 1, 2, 4, 5, 6, 7, 0 };
    static const uint32_t a71a[] = { 3, 7, 8, 1, 2, 5, 6, 4, 0 };
    static const uint32_t a71c[] = { 3, 1, 2, 7, 8, 5, 6, 4, 0 };
    static const uint32_t a80[] = { 3, 1, 2, 4, 5, 7, 8, 6, 0 };
    const uint32_t *a = 0;
    uint32_t newtag = 0;
    switch (layout->mChannelLayoutTag) {
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
    case kAudioChannelLayoutTag_MPEG_7_1_C:
	a = a71c; newtag = kAudioChannelLayoutTag_AAC_7_1; break;
    case kAudioChannelLayoutTag_UseChannelBitmap:
	if (layout->mChannelBitmap == 0x137) {
	    a = a60; newtag = kAudioChannelLayoutTag_AAC_6_0;
	} else if (layout->mChannelBitmap == 0x637) {
	    a = a70; newtag = kAudioChannelLayoutTag_AAC_7_0;
	} else if (layout->mChannelBitmap == 0x737) {
	    a = a80; newtag = kAudioChannelLayoutTag_AAC_Octagonal;
	}
	break;
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
