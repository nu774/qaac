#include <CoreAudioTypes.h>
#include "chanmap.h"
#include "util.h"

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
