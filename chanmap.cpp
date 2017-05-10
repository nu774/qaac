#include "chanmap.h"
#include <numeric>
#include <cassert>
#include "strutil.h"

namespace chanmap {

const char *getChannelName(uint32_t n)
{
    const char *tab[] = {
        "?","L","R","C","LFE","Ls","Rs","Lc","Rc","Cs",
        "Lsd","Rsd","Ts","Vhl","Vhc","Vhr","Tbl","Tbc","Tbr"
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

std::string getChannelNames(const std::vector<uint32_t> &channels)
{
    const char *delim = "";
    std::string result;

    std::for_each(channels.begin(), channels.end(), [&](uint32_t c) {
        result.append(delim).append(getChannelName(c));
        delim = " ";
    });
    size_t lfe_count = std::count(channels.begin(), channels.end(), 4);
    size_t count = channels.size() - lfe_count;
    if (count <= 2 && lfe_count == 0)
        return count == 1 ? "Mono" : "Stereo";
    else
        return strutil::format("%u.%u (%s)",
                               static_cast<uint32_t>(count),
                               static_cast<uint32_t>(lfe_count),
                               result.c_str());
}

uint32_t getChannelMask(const std::vector<uint32_t>& channels)
{
    if (std::count_if(channels.begin(), channels.end(), [](uint32_t c) {
                      return c >= 33; }))
        throw std::runtime_error("Not supported channel layout");

    return std::accumulate(channels.begin(), channels.end(), 0,
                           [](uint32_t a, uint32_t c) {
                               return a | (1 << (c - 1));
                           });
}

std::vector<uint32_t> getChannels(uint32_t bitmap, uint32_t limit)
{
    std::vector<uint32_t> channels;
    for (unsigned i = 0; i < 32 && channels.size() < limit; ++i) {
        if (bitmap & (1<<i))
            channels.push_back(i + 1);
    }
    return channels;
}

std::vector<uint32_t> getChannels(const AudioChannelLayout *acl)
{
    std::vector<uint32_t> channels;
    uint32_t bitmap = 0;
    const char *layout = 0;

    switch (acl->mChannelLayoutTag) {
    case kAudioChannelLayoutTag_UseChannelBitmap:
        return getChannels(acl->mChannelBitmap);
    case kAudioChannelLayoutTag_UseChannelDescriptions:
    {
        const AudioChannelDescription *desc = acl->mChannelDescriptions;
        for (size_t i = 0; i < acl->mNumberChannelDescriptions; ++i)
            channels.push_back(desc[i].mChannelLabel);
        return channels;
    }
    /* 1ch */
    case kAudioChannelLayoutTag_Mono:
        layout = "\x2a"; break; /* kAudioChannelLabel_Mono */
    /* 1.1ch */
    case kAudioChannelLayoutTag_AC3_1_0_1:
        layout = "\x03\x04"; break;
    /* 2ch */
    case kAudioChannelLayoutTag_Stereo:
    case kAudioChannelLayoutTag_MatrixStereo: /* XXX: Actually Lt+Rt */
    case kAudioChannelLayoutTag_Binaural:
        layout = "\x01\x02"; break;
    /* 2.1ch */
    case kAudioChannelLayoutTag_DVD_4:
        layout = "\x01\x02\x04"; break;
    /* 3ch */
    case kAudioChannelLayoutTag_MPEG_3_0_A:
        layout = "\x01\x02\x03"; break;
    case kAudioChannelLayoutTag_AC3_3_0:
        layout = "\x01\x03\x02"; break;
    case kAudioChannelLayoutTag_MPEG_3_0_B:
        layout = "\x03\x01\x02"; break;
    case kAudioChannelLayoutTag_ITU_2_1:
        layout = "\x01\x02\x09"; break;
    /* 3.1ch */
    case kAudioChannelLayoutTag_DVD_10:
        layout = "\x01\x02\x03\x04"; break;
    case kAudioChannelLayoutTag_AC3_3_0_1:
        layout = "\x01\x03\x02\x04"; break;
    case kAudioChannelLayoutTag_DVD_5:
        layout = "\x01\x02\x04\x09"; break;
    case kAudioChannelLayoutTag_AC3_2_1_1:
        layout = "\x01\x02\x09\x04"; break;
    case kAudioChannelLayoutTag_DTS_3_1:
        layout = "\x03\x01\x02\x04"; break;
    /* 4ch */
    case kAudioChannelLayoutTag_Quadraphonic:
    case kAudioChannelLayoutTag_ITU_2_2:
        layout = "\x01\x02\x05\x06"; break;
    case kAudioChannelLayoutTag_MPEG_4_0_A:
        layout = "\x01\x02\x03\x09"; break;
    case kAudioChannelLayoutTag_MPEG_4_0_B:
        layout = "\x03\x01\x02\x09"; break;
    case kAudioChannelLayoutTag_AC3_3_1:
        layout = "\x01\x03\x02\x09"; break;
    /* 4.1ch */
    case kAudioChannelLayoutTag_DVD_6:
        layout = "\x01\x02\x04\x05\x06"; break;
    case kAudioChannelLayoutTag_DVD_18:
        layout = "\x01\x02\x05\x06\x04"; break;
    case kAudioChannelLayoutTag_DVD_11:
        layout = "\x01\x02\x03\x04\x09"; break;
    case kAudioChannelLayoutTag_AC3_3_1_1:
        layout = "\x01\x03\x02\x09\x04"; break;
    case kAudioChannelLayoutTag_DTS_4_1:
        layout = "\x03\x01\x02\x09\x04"; break;
    /* 5ch */
    case kAudioChannelLayoutTag_MPEG_5_0_A:
        layout = "\x01\x02\x03\x05\x06"; break;
    case kAudioChannelLayoutTag_Pentagonal:
    case kAudioChannelLayoutTag_MPEG_5_0_B:
        layout = "\x01\x02\x05\x06\x03"; break;
    case kAudioChannelLayoutTag_MPEG_5_0_C:
        layout = "\x01\x03\x02\x05\x06"; break;
    case kAudioChannelLayoutTag_MPEG_5_0_D:
        layout = "\x03\x01\x02\x05\x06"; break;
    /* 5.1ch */
    case kAudioChannelLayoutTag_MPEG_5_1_A:
        layout = "\x01\x02\x03\x04\x05\x06"; break;
    case kAudioChannelLayoutTag_MPEG_5_1_B:
        layout = "\x01\x02\x05\x06\x03\x04"; break;
    case kAudioChannelLayoutTag_MPEG_5_1_C:
        layout = "\x01\x03\x02\x05\x06\x04"; break;
    case kAudioChannelLayoutTag_MPEG_5_1_D:
        layout = "\x03\x01\x02\x05\x06\x04"; break;
    /* 6ch */
    case kAudioChannelLayoutTag_Hexagonal:
    case kAudioChannelLayoutTag_AudioUnit_6_0:
        layout = "\x01\x02\x05\x06\x03\x09"; break;
    case kAudioChannelLayoutTag_AAC_6_0:
        layout = "\x03\x01\x02\x05\x06\x09"; break;
    case kAudioChannelLayoutTag_EAC_6_0_A:
        layout = "\x01\x03\x02\x05\x06\x09"; break;
    case kAudioChannelLayoutTag_DTS_6_0_A:
        layout = "\x07\x08\x01\x02\x05\x06"; break;
    case kAudioChannelLayoutTag_DTS_6_0_B:
        layout = "\x03\x01\x02\x21\x22\x0C"; break;
    case kAudioChannelLayoutTag_DTS_6_0_C:
        layout = "\x03\x09\x01\x02\x21\x22"; break;
    /* 6.1ch */
    case kAudioChannelLayoutTag_MPEG_6_1_A:
        layout = "\x01\x02\x03\x04\x05\x06\x09"; break;
    case kAudioChannelLayoutTag_AAC_6_1:
        layout = "\x03\x01\x02\x05\x06\x09\x04"; break;
    case kAudioChannelLayoutTag_EAC3_6_1_A:
        layout = "\x01\x03\x02\x05\x06\x04\x09"; break;
    case kAudioChannelLayoutTag_EAC3_6_1_B:
        layout = "\x01\x03\x02\x05\x06\x04\x0C"; break;
    case kAudioChannelLayoutTag_EAC3_6_1_C:
        layout = "\x01\x03\x02\x05\x06\x04\x0E"; break;
    case kAudioChannelLayoutTag_DTS_6_1_A:
        layout = "\x07\x08\x01\x02\x05\x06\x04"; break;
    case kAudioChannelLayoutTag_DTS_6_1_B:
        layout = "\x03\x01\x02\x21\x22\x0C\x04"; break;
    case kAudioChannelLayoutTag_DTS_6_1_C:
        layout = "\x03\x09\x01\x02\x21\x22\x04"; break;
    case kAudioChannelLayoutTag_DTS_6_1_D:
        layout = "\x03\x01\x02\x05\x06\x04\x09"; break;
    /* 7ch */
    case kAudioChannelLayoutTag_AudioUnit_7_0:
        layout = "\x01\x02\x05\x06\x03\x21\x22"; break;
    case kAudioChannelLayoutTag_AudioUnit_7_0_Front:
        layout = "\x01\x02\x05\x06\x03\x07\x08"; break;
    case kAudioChannelLayoutTag_AAC_7_0:
        layout = "\x03\x01\x02\x05\x06\x21\x22"; break;
    case kAudioChannelLayoutTag_EAC_7_0_A:
        layout = "\x01\x03\x02\x05\x06\x21\x22"; break;
    case kAudioChannelLayoutTag_DTS_7_0:
        layout = "\x07\x03\x08\x01\x02\x05\x06"; break;
    /* 7.1ch */
    case kAudioChannelLayoutTag_MPEG_7_1_A:
        layout = "\x01\x02\x03\x04\x05\x06\x07\x08"; break;
    case kAudioChannelLayoutTag_MPEG_7_1_B:
        layout = "\x03\x07\x08\x01\x02\x05\x06\x04"; break;
    case kAudioChannelLayoutTag_MPEG_7_1_C:
        layout = "\x01\x02\x03\x04\x05\x06\x21\x22"; break;
    case kAudioChannelLayoutTag_Emagic_Default_7_1:
        layout = "\x01\x02\x05\x06\x03\x04\x07\x08"; break;
    case kAudioChannelLayoutTag_AAC_7_1_B:
        layout = "\x03\x01\x02\x05\x06\x21\x22\x04"; break;
    case kAudioChannelLayoutTag_AAC_7_1_C:
        layout = "\x03\x01\x02\x05\x06\x04\x0D\x0F"; break;
    case kAudioChannelLayoutTag_EAC3_7_1_A:
        layout = "\x01\x03\x02\x05\x06\x04\x21\x22"; break;
    case kAudioChannelLayoutTag_EAC3_7_1_B:
        layout = "\x01\x03\x02\x05\x06\x04\x07\x08"; break;
    case kAudioChannelLayoutTag_EAC3_7_1_C:
        layout = "\x01\x03\x02\x05\x06\x04\x0A\x0B"; break;
    case kAudioChannelLayoutTag_EAC3_7_1_D:
        layout = "\x01\x03\x02\x05\x06\x04\x23\x24"; break;
    case kAudioChannelLayoutTag_EAC3_7_1_E:
        layout = "\x01\x03\x02\x05\x06\x04\x0D\x0F"; break;
    case kAudioChannelLayoutTag_EAC3_7_1_F:
        layout = "\x01\x03\x02\x05\x06\x04\x09\x0C"; break;
    case kAudioChannelLayoutTag_EAC3_7_1_G:
        layout = "\x01\x03\x02\x05\x06\x04\x09\x0E"; break;
    case kAudioChannelLayoutTag_EAC3_7_1_H:
        layout = "\x01\x03\x02\x05\x06\x04\x0C\x0E"; break;
    case kAudioChannelLayoutTag_DTS_7_1:
        layout = "\x07\x03\x08\x01\x02\x05\x06\x04"; break;
    /* 8ch */
    case kAudioChannelLayoutTag_Octagonal:
        /* XXX: actually the last two are Left Wide/Right Wide */
        layout = "\x01\x02\x05\x06\x03\x09\x0A\x0B"; break;
    case kAudioChannelLayoutTag_AAC_Octagonal:
        layout = "\x03\x01\x02\x05\x06\x21\x22\x09"; break;
    case kAudioChannelLayoutTag_DTS_8_0_A:
        layout = "\x07\x08\x01\x02\x05\x06\x21\x22"; break;
    case kAudioChannelLayoutTag_DTS_8_0_B:
        layout = "\x07\x03\x08\x01\x02\x05\x09\x06"; break;
    default:
        throw std::runtime_error("Unsupported channel layout");
    }

    while (*layout) channels.push_back(*layout++);
    return channels;
}

std::vector<uint32_t>
convertFromAppleLayout(const std::vector<uint32_t> &channels)
{
    if (!std::count_if(channels.begin(), channels.end(),
                       [](uint32_t c) {
                           return c > 18;
                       }))
        return channels;

    std::vector<uint32_t> v(channels.size());
    std::transform(channels.begin(), channels.end(), v.begin(),
                   [](uint32_t x) -> uint32_t {
                       switch (x) {
                       case kAudioChannelLabel_Mono:
                           return kAudioChannelLabel_Center;
                       case kAudioChannelLabel_HeadphonesLeft:
                           return kAudioChannelLabel_Left;
                       case kAudioChannelLabel_HeadphonesRight:
                           return kAudioChannelLabel_Right;
                       }
                       return x;
                   });

    size_t back = std::count(v.begin(), v.end(), 5) +
                  std::count(v.begin(), v.end(), 6);
    size_t side = std::count(v.begin(), v.end(), 10) +
                  std::count(v.begin(), v.end(), 11);

    std::transform(v.begin(), v.end(), v.begin(), [&](uint32_t c) -> uint32_t {
                        switch (c) {
                        case kAudioChannelLabel_LeftSurround:
                        case kAudioChannelLabel_RightSurround:
                            if (!side) c += 5;
                            break;
                        case kAudioChannelLabel_RearSurroundLeft:
                        case kAudioChannelLabel_RearSurroundRight:
                            if (!back || !side) c -= 28;
                            break;
                        };
                        return c;
                   });
    return v;
}

std::vector<uint32_t>
getMappingToUSBOrder(const std::vector<uint32_t> &channels)
{
    std::vector<uint32_t> index(channels.size());
    std::iota(index.begin(), index.end(), 1);
    std::sort(index.begin(), index.end(), [&](uint32_t a, uint32_t b) {
                  return channels[a-1] < channels[b-1];
              });
    return index;
}

uint32_t defaultChannelMask(const uint32_t nchannels)
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

uint32_t AACLayoutFromBitmap(uint32_t bitmap)
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
    case 0x63f: return kAudioChannelLayoutTag_AAC_7_1_B;
    case 0x737: return kAudioChannelLayoutTag_AAC_Octagonal;
    }
    throw std::runtime_error("No channel mapping to AAC defined");
}

std::vector<uint32_t> getMappingToAAC(uint32_t bitmap)
{
    AudioChannelLayout aacLayout = { 0 };
    aacLayout.mChannelLayoutTag = AACLayoutFromBitmap(bitmap);
    auto cs = getChannels(&aacLayout);
    /*
     * rewrite channels in pre-defined AAC channel layout to match with
     * input channel bitmap
     */
    std::transform(cs.begin(), cs.end(), cs.begin(),
                   [&](uint32_t c) -> uint32_t {
                        switch (c) {
                        case kAudioChannelLabel_Mono:
                            c = kAudioChannelLabel_Center;
                            break;
                        case kAudioChannelLabel_Left:
                        case kAudioChannelLabel_Right:
                            if (!(bitmap & 0x3) && (bitmap & 0xc))
                                c += 6;
                            break;
                        case kAudioChannelLabel_LeftSurround:
                        case kAudioChannelLabel_RightSurround:
                            if (bitmap & 0x600)
                                c += 5;
                            break;
                        case kAudioChannelLabel_RearSurroundLeft:
                        case kAudioChannelLabel_RearSurroundRight:
                            c -= 28;
                            break;
                        }
                        return c;
                  });
    assert(getChannelMask(cs) == bitmap);
    auto mapping = getMappingToUSBOrder(cs);
    mapping = getMappingToUSBOrder(mapping);
    return mapping;
}

} // namespace
