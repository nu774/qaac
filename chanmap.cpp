#include <CoreAudioTypes.h>
#include "chanmap.h"

uint32_t
LayoutToChannelMask(const std::vector<uint32_t>& chanmap)
{
    uint32_t result = 0;
    for (size_t i = 0; i < chanmap.size(); ++i)
	result |= (1 << (chanmap[i] - 1));
    return result;
}

uint32_t
GetChannelLayoutTagFromChannelMap(const std::vector<uint32_t>& chanmap)
{
    switch (LayoutToChannelMask(chanmap)) {
    case 0x1: return kAudioChannelLayoutTag_Mono;
    case 0x3: return kAudioChannelLayoutTag_Stereo;
    case 0x7: return kAudioChannelLayoutTag_MPEG_3_0_A;
    case 0x33: return kAudioChannelLayoutTag_Quadraphonic;
    case 0x107: return kAudioChannelLayoutTag_MPEG_4_0_A;
    case 0x37: case 0x607:
	return kAudioChannelLayoutTag_MPEG_5_0_A;
    case 0x3f: case 0x60f:
	return kAudioChannelLayoutTag_MPEG_5_1_A;
    case 0x13f: case 0x70f:
	return kAudioChannelLayoutTag_MPEG_6_1_A;
    case 0xff: case 0x63f: return kAudioChannelLayoutTag_MPEG_7_1_A;
    }
    throw std::runtime_error("Sorry, this channel layout not supported");
}

uint32_t GetAACChannelMapFromLayoutTag(uint32_t tag,
	std::vector<uint32_t> *result)
{
    static const uint32_t a30[] = { 3, 1, 2, 0 };
    static const uint32_t a40[] = { 3, 1, 2, 4, 0 };
    static const uint32_t a50[] = { 3, 1, 2, 4, 5, 0 };
    static const uint32_t a51[] = { 3, 1, 2, 5, 6, 4, 0 };
    static const uint32_t a61[] = { 3, 1, 2, 5, 6, 7, 4, 0 };
    static const uint32_t a71[] = { 3, 7, 8, 1, 2, 5, 6, 4, 0 };
    const uint32_t *a = 0;
    uint32_t newtag = 0;
    switch (tag) {
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
	a = a71; newtag = kAudioChannelLayoutTag_AAC_7_1; break;
    }
    while (a && *a) result->push_back(*a++);
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
