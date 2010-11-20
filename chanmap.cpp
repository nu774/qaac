#include <CoreAudioTypes.h>
#include "chanmap.h"
#include "util.h"

#if 0
static const uint32_t MPEG_3_0_A[] = { 1, 2, 3 };
static const uint32_t MPEG_3_0_B[] = { 3, 1, 2 };
static const uint32_t ITU_2_1[] = { 1, 2, 9 };
static const uint32_t DVD_4[] = { 1, 2, 4 };

static const uint32_t *Layout3[] = { MPEG_3_0_A, MPEG_3_0_B, ITU_2_1, DVD_4 };
static const uint32_t Tags3[] = { 
    kAudioChannelLayoutTag_MPEG_3_0_A,
    kAudioChannelLayoutTag_MPEG_3_0_B,
    kAudioChannelLayoutTag_ITU_2_1,
    kAudioChannelLayoutTag_DVD_4
};

static const uint32_t Quadraphonic[] = { 1, 2, 5, 6 };
static const uint32_t MPEG_4_0_A[] = { 1, 2, 3, 9 };
static const uint32_t MPEG_4_0_B[] = { 3, 1, 2, 9 };
static const uint32_t DVD_5[] = { 1, 2, 4, 9 };
static const uint32_t DVD_10[] = { 1, 2, 3, 4 };
static const uint32_t *Layout4[] = {
    Quadraphonic, MPEG_4_0_A, MPEG_4_0_B, DVD_5, DVD_10
};
static const uint32_t Tags4[] = {
    kAudioChannelLayoutTag_Quadraphonic,
    kAudioChannelLayoutTag_MPEG_4_0_A,
    kAudioChannelLayoutTag_MPEG_4_0_B,
    kAudioChannelLayoutTag_DVD_5,
    kAudioChannelLayoutTag_DVD_10
};

static const uint32_t MPEG_5_0_A[] = { 1, 2, 3, 5, 6 };
static const uint32_t MPEG_5_0_B[] = { 1, 2, 5, 6, 3 };
static const uint32_t MPEG_5_0_C[] = { 1, 3, 2, 5, 6 };
static const uint32_t MPEG_5_0_D[] = { 3, 1, 2, 5, 6 };
static const uint32_t DVD_6[] = { 1, 2, 4, 5, 6 };
static const uint32_t DVD_11[] = { 1, 2, 3, 4, 9 };
static const uint32_t DVD_18[] = { 1, 2, 5, 6, 4 };
static const uint32_t *Layout5[] = {
    MPEG_5_0_A, MPEG_5_0_B, MPEG_5_0_C, MPEG_5_0_D, DVD_6, DVD_11, DVD_18
};
static const uint32_t Tags5[] = {
    kAudioChannelLayoutTag_MPEG_5_0_A,
    kAudioChannelLayoutTag_MPEG_5_0_B,
    kAudioChannelLayoutTag_MPEG_5_0_C,
    kAudioChannelLayoutTag_MPEG_5_0_D,
    kAudioChannelLayoutTag_DVD_6,
    kAudioChannelLayoutTag_DVD_11,
    kAudioChannelLayoutTag_DVD_18
};

static const uint32_t Hexagonal[] = { 1, 2, 5, 6, 3, 9 };
static const uint32_t MPEG_5_1_A[] = { 1, 2, 3, 4, 5, 6 };
static const uint32_t MPEG_5_1_B[] = { 1, 2, 4, 5, 3, 6 };
static const uint32_t MPEG_5_1_C[] = { 1, 3, 2, 5, 6, 4 };
static const uint32_t MPEG_5_1_D[] = { 3, 1, 2, 5, 6, 4 };
static const uint32_t AAC_6_0[] = { 3, 1, 2, 5, 6, 9 };
static const uint32_t *Layout6[] = {
    Hexagonal, MPEG_5_1_A, MPEG_5_1_B, MPEG_5_1_C, MPEG_5_1_D, AAC_6_0
};
static const uint32_t Tags6[] = {
    kAudioChannelLayoutTag_Hexagonal,
    kAudioChannelLayoutTag_MPEG_5_1_A,
    kAudioChannelLayoutTag_MPEG_5_1_B,
    kAudioChannelLayoutTag_MPEG_5_1_C,
    kAudioChannelLayoutTag_MPEG_5_1_D,
    kAudioChannelLayoutTag_AAC_6_0
};

static const uint32_t MPEG_6_1_A[] = { 1, 2, 3, 4, 5, 6, 9 };
static const uint32_t AudioUnit_7_0[] = { 1, 2, 5, 6, 3, 33, 34 };
static const uint32_t AAC_6_1[] = { 3, 1, 2, 5, 6, 9, 4 };
static const uint32_t AAC_7_0[] = { 3, 1, 2, 5, 6, 33, 34};
static const uint32_t *Layout7[] = {
    MPEG_6_1_A, AudioUnit_7_0, AAC_6_1, AAC_7_0
};
static const uint32_t Tags7[] = {
    kAudioChannelLayoutTag_MPEG_6_1_A,
    kAudioChannelLayoutTag_AudioUnit_7_0,
    kAudioChannelLayoutTag_AAC_6_1,
    kAudioChannelLayoutTag_AAC_7_0
};

static const uint32_t Octagonal[] = { 1, 2, 5, 6, 3, 9, 7, 8 };
static const uint32_t MPEG_7_1_A[] = { 1, 2, 3, 4, 5, 6, 7, 8 };
static const uint32_t MPEG_7_1_B[] = { 3, 7, 8, 1, 2, 5, 6, 4 };
static const uint32_t MPEG_7_1_C[] = { 1, 2, 3, 4, 5, 6, 33, 34 };
static const uint32_t Emagic_Default_7_1[] = { 1, 2, 5, 6, 3, 4, 7, 8 };
static const uint32_t AAC_Octagonal[] = { 3, 1, 2, 5, 6, 33, 34, 9 };
static const uint32_t *Layout8[] = {
    Octagonal, MPEG_7_1_A, MPEG_7_1_B, MPEG_7_1_C, Emagic_Default_7_1,
    AAC_Octagonal
};
static const uint32_t Tags8[] = {
    kAudioChannelLayoutTag_Octagonal,
    kAudioChannelLayoutTag_MPEG_7_1_A,
    kAudioChannelLayoutTag_MPEG_7_1_B,
    kAudioChannelLayoutTag_MPEG_7_1_C,
    kAudioChannelLayoutTag_Emagic_Default_7_1,
    kAudioChannelLayoutTag_AAC_Octagonal
};

static const size_t LayoutsCount[] = {
    array_size(Layout3), array_size(Layout4), array_size(Layout5),
    array_size(Layout6), array_size(Layout7), array_size(Layout8)
};
static const uint32_t * const *Layouts[] = {
    Layout3, Layout4, Layout5, Layout6, Layout7, Layout8
};
static const uint32_t *Tags[] = {
    Tags3, Tags4, Tags5, Tags6, Tags7, Tags8
};
#endif

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
