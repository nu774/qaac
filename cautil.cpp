#include "cautil.h"
#include "dl.h"
#include "bitstream.h"

namespace cautil {
    std::string make_coreaudio_error(long code, const char *s)
    {
        std::stringstream ss;
        if (code == FOURCC('t','y','p','?'))
            return "Unsupported file type";
        else if (code == FOURCC('f','m','t','?'))
            return "Data format is not supported for this file type";
        int shift;
        for (shift = 0; shift < 32; shift += 8)
            if (!isprint((code >> shift) & 0xff))
                break;
        if (shift == 32)
            ss << s << ": "
               << static_cast<char>(code >> 24)
               << static_cast<char>((code >> 16) & 0xff)
               << static_cast<char>((code >> 8) & 0xff)
               << static_cast<char>(code & 0xff);
        else
            ss << s << ": " << code;
        return ss.str();
    }

#ifndef NO_COREAUDIO
    CFMutableDictionaryRef CreateDictionary(CFIndex capacity)
    {
        static CFDictionaryKeyCallBacks *keyCB;
        static CFDictionaryValueCallBacks *valueCB;
        if (!keyCB) {
            DL dll(GetModuleHandleA("CoreFoundation.dll"), false);
            CFDictionaryKeyCallBacks *kcb =
                dll.fetch("kCFTypeDictionaryKeyCallBacks");
            CFDictionaryValueCallBacks *vcb =
                dll.fetch("kCFTypeDictionaryValueCallBacks");
            InterlockedCompareExchangePointerRelease(
                     reinterpret_cast<LPVOID*>(&keyCB), kcb, 0);
            InterlockedCompareExchangePointerRelease(
                     reinterpret_cast<LPVOID*>(&valueCB), vcb, 0);
        }
        return CFDictionaryCreateMutable(0, capacity, keyCB, valueCB);
    }
#endif

    AudioStreamBasicDescription
        buildASBDForPCM(double sample_rate, unsigned channels_per_frame,
                        unsigned bits_per_channel, unsigned type,
                        unsigned alignment)
    {
        AudioStreamBasicDescription asbd = { 0 };
        asbd.mFormatID = 'lpcm';
        asbd.mFormatFlags = type;
        if (bits_per_channel & 0x7)
            asbd.mFormatFlags |= alignment;
        else
            asbd.mFormatFlags |= kAudioFormatFlagIsPacked;
        asbd.mSampleRate = sample_rate;
        asbd.mChannelsPerFrame = channels_per_frame;
        asbd.mBitsPerChannel = bits_per_channel;
        asbd.mFramesPerPacket = 1;
        asbd.mBytesPerFrame =
            asbd.mChannelsPerFrame * ((asbd.mBitsPerChannel + 7) & ~7) / 8;
        asbd.mBytesPerPacket = asbd.mFramesPerPacket * asbd.mBytesPerFrame;
        return asbd;
    }

    AudioStreamBasicDescription
        buildASBDForPCM2(double sample_rate, unsigned channels_per_frame,
                         unsigned valid_bits, unsigned pack_bits,
                         unsigned type, unsigned alignment)
    {
        AudioStreamBasicDescription asbd = { 0 };
        asbd.mFormatID = 'lpcm';
        asbd.mFormatFlags = type;
        if (valid_bits != pack_bits)
            asbd.mFormatFlags |= alignment;
        else
            asbd.mFormatFlags |= kAudioFormatFlagIsPacked;
        asbd.mSampleRate = sample_rate;
        asbd.mChannelsPerFrame = channels_per_frame;
        asbd.mBitsPerChannel = valid_bits;
        asbd.mFramesPerPacket = 1;
        asbd.mBytesPerFrame = asbd.mChannelsPerFrame * pack_bits / 8;
        asbd.mBytesPerPacket = asbd.mFramesPerPacket * asbd.mBytesPerFrame;
        return asbd;
    }

    const uint8_t *getDescripterHeader(const uint8_t *p, const uint8_t *end,
                                       int *tag, uint32_t *size)
    {
        const uint8_t *q = p;
        *size = 0;
        if (q < end) {
            *tag = *q++;
            while (q < end) {
                int n = *q++;
                *size = (*size << 7) | (n & 0x7f);
                if (!(n & 0x80)) return q;
            }
        }
        return 0;
    }
    std::vector<uint8_t> parseMagicCookieAAC(const std::vector<uint8_t> &cookie)
    {
        /*
         * QT's "Magic Cookie" for AAC is just an esds descripter.
         * We obtain only decSpecificConfig from it, and discard others.
         */
        const uint8_t *p = &cookie[0], *q;
        const uint8_t *end = p + cookie.size();
        int tag;
        uint32_t size;
        for (;(q = getDescripterHeader(p, end, &tag, &size)); p = q) {
            switch (tag) {
            case 3: // esds
                /*
                 * ES_ID: 16
                 * streamDependenceFlag: 1
                 * URLFlag: 1
                 * OCRstreamFlag: 1
                 * streamPriority: 5
                 *
                 * (flags are all zero, so other atttributes are not present)
                 */
                q += 3;
                break;
            case 4: // decConfig
                /*
                 * objectTypeId: 8
                 * streamType: 6
                 * upStream: 1
                 * reserved: 1
                 * bufferSizeDB: 24
                 * maxBitrate: 32
                 * avgBitrate: 32
                 *
                 * QT gives constant value for bufferSizeDB, max/avgBitrate
                 * depending on encoder settings.
                 * On the other hand, mp4v2 sets decConfig from
                 * actually computed values when finished media writing.
                 * Therefore, these values will be different from QT.
                 */
                q += 13;
                break;
            case 5: // decSpecificConfig
                {
                    return std::vector<uint8_t>(q, q + size);
                }
            default:
                q += size;
            }
        }
        throw std::runtime_error(
                "Magic cookie format is different from expected!!");
    }
    void replaceASCInMagicCookie(std::vector<uint8_t> *cookie,
                              const std::vector<uint8_t> &data)
    {
        std::vector<uint8_t> configs;
        const uint8_t *p = cookie->data(), *q;
        const uint8_t *end = p + cookie->size();
        int tag;
        uint32_t size;
        size_t esds_pos, decConfig_pos, decConfig_end;

        for (;(q = getDescripterHeader(p, end, &tag, &size)); p = q) {
            switch (tag) {
            case 3:
                esds_pos = configs.size();
                std::copy(p, q + 3, std::back_inserter(configs));
                q += 3;
                break;
            case 4:
                decConfig_pos = configs.size();
                std::copy(p, q + 13, std::back_inserter(configs));
                q += 13;
                break;
            case 5:
                configs.push_back(5);
                configs.push_back((data.size() >> 21) | 0x80);
                configs.push_back((data.size() >> 14) | 0x80);
                configs.push_back((data.size() >>  7) | 0x80);
                configs.push_back((data.size() & 0x7f));
                std::copy(data.begin(), data.end(),
                          std::back_inserter(configs));
                decConfig_end = configs.size();
                q += size;
                break;
            default:
                std::copy(p, q + size, std::back_inserter(configs));
                q += size;
            }
        }
        size_t esds_size = configs.size() - esds_pos - 5;
        size_t decConfig_size = decConfig_end - decConfig_pos - 5;
        configs[esds_pos + 1] = ((esds_size >> 21) | 0x80);
        configs[esds_pos + 2] = ((esds_size >> 14) | 0x80);
        configs[esds_pos + 3] = ((esds_size >>  7) | 0x80);
        configs[esds_pos + 4] = (esds_size & 0x7F);
        configs[decConfig_pos + 1] = ((decConfig_size >> 21) | 0x80);
        configs[decConfig_pos + 2] = ((decConfig_size >> 14) | 0x80);
        configs[decConfig_pos + 3] = ((decConfig_size >>  7) | 0x80);
        configs[decConfig_pos + 4] = (decConfig_size & 0x7F);

        cookie->swap(configs);
    }

    void parseASC(const std::vector<uint8_t> &asc,
                  AudioStreamBasicDescription *asbd,
                  std::vector<uint32_t> *channels)
    {
        BitStream bs(asc.data(), asc.size());
        uint8_t aot = bs.get(5);
        if (aot != 2 && aot != 5 && aot != 29)
            throw std::runtime_error("Unsupported AudioSpecificConfig");
        static const unsigned sftab[] = {
            96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050, 
            16000, 12000, 11025, 8000, 7350, 0, 0, 0
        };
        uint32_t sample_rate    = sftab[bs.get(4)];
        uint8_t  chan_config    = bs.get(4);
        if (aot == 5 || aot == 29) {
            sample_rate = sftab[bs.get(4)];
            bs.advance(5); // AOT
        }
        // GASpecificConfig
        bs.advance(1); // frameLengthFlag
        if (bs.get(1)) bs.advance(14); // dependsOnCoreCoder
        bs.advance(1); // extensionFlag
        const char *ch_layout_tab[16] = {
            0,
            "\x03",
            "\x01\x02",
            "\x03\x01\x02",
            "\x03\x01\x02\x09",
            "\x03\x01\x02\x05\x06",
            "\x03\x01\x02\x05\x06\x04",
            "\x03\x07\x08\x01\x02\x05\x06\x04",
            0,
            0,
            0,
            "\x03\x01\x02\x05\x06\x09\x04",
            "\x03\x01\x02\x05\x06\x21\x22\x04",
            0,
        };
        if (chan_config) {
            const char *lp = ch_layout_tab[chan_config];
            if (lp) {
                std::vector<uint32_t> v;
                while (*lp) v.push_back(*lp++); 
                channels->swap(v);
            }
        } else { // PCE
            bs.advance(10); // element_instance_tag, object_type, sf_index
            uint8_t nfront  = bs.get(4);
            uint8_t nside   = bs.get(4);
            uint8_t nback   = bs.get(4);
            uint8_t nlfe    = bs.get(2);
            uint8_t nassoc  = bs.get(3);
            uint8_t ncc     = bs.get(4);
            if (bs.get(1)) bs.get(4); // mono_mixdown
            if (bs.get(1)) bs.get(4); // stereo_mixdown
            if (bs.get(1)) bs.get(3); // matrix_mixdown
            uint8_t nfront_channels = 0;
            uint8_t nside_channels  = 0;
            uint8_t nback_channels  = 0;
            for (uint8_t i = 0; i < nfront; ++i) {
                if (bs.get(1)) // is_cpe
                    nfront_channels += 2;
                else
                    nfront_channels += 1;
                bs.advance(4); // element_tag_select
            }
            for (uint8_t i = 0; i < nside; ++i) {
                if (bs.get(1)) // is_cpe
                    nside_channels += 2;
                else
                    nside_channels += 1;
                bs.advance(4); // element_tag_select
            }
            for (uint8_t i = 0; i < nback; ++i) {
                if (bs.get(1)) // is_cpe
                    nback_channels += 2;
                else
                    nback_channels += 1;
                bs.advance(4); // element_tag_select
            }
            for (uint8_t i = 0; i < nlfe; ++i)
                bs.advance(4);
            for (uint8_t i = 0; i < nassoc; ++i)
                bs.advance(4);
            for (uint8_t i = 0; i < ncc; ++i)
                bs.advance(5);
            {
                std::vector<uint32_t> v;
                if (nfront_channels & 1) {
                    v.push_back(3);
                    --nfront_channels;
                }
                if (nfront_channels > 3) {
                    v.push_back(7);
                    v.push_back(8);
                    nfront_channels -= 2;
                }
                if (nfront_channels > 1) {
                    v.push_back(1);
                    v.push_back(2);
                    nfront_channels -= 2;
                }
                nside_channels += nback_channels;
                if (nside_channels > 1) {
                    v.push_back(5);
                    v.push_back(6);
                    nside_channels -= 2;
                }
                if (nside_channels > 1) {
                    v.push_back(33);
                    v.push_back(34);
                    nside_channels -= 2;
                }
                if (nside_channels & 1) {
                    v.push_back(9);
                    --nside_channels;
                }
                if (nlfe) {
                    v.push_back(4);
                    --nlfe;
                }
                if (nfront_channels || nside_channels || nlfe)
                    throw std::runtime_error("Unsupported channel layout");
                channels->swap(v);
            }
            // byte align
            bs.advance((8 - (bs.position() & 7)) & 7);
            uint8_t comment_len = bs.get(1);
            bs.advance(8 * comment_len);
        }
        if (asc.size() * 8 - bs.position() >= 16) {
            if (bs.get(11) == 0x2b7) {
                uint8_t tmp = bs.get(5);
                if (tmp == 5 && bs.get(1)) {
                    aot = tmp;
                    sample_rate = sftab[bs.get(4)];
                }
                if (asc.size() * 8 - bs.position() >= 12) {
                    if (bs.get(11) == 0x548 && bs.get(1))
                        aot = 29;
                }
            }
        }
        if (aot == 29) {
            std::vector<uint32_t> v;
            v.push_back(1);
            v.push_back(2);
            channels->swap(v);
        }
        memset(asbd, 0, sizeof(AudioStreamBasicDescription));
        asbd->mSampleRate = sample_rate;
        asbd->mFormatID   = (aot == 2) ? 'aac '
                                       : (aot == 5) ? 'aach' : 'aacp';
        asbd->mFramesPerPacket  = (aot == 2) ? 1024 : 2048;
        asbd->mChannelsPerFrame = channels->size();
    }

    void insert71RearPCEToASC(std::vector<uint8_t> *asc)
    {
        BitStream ibs(asc->data(), asc->size());
        BitStream bs;
        bs.copy(ibs, 5);  /* obj_type */
        uint32_t sf_index = bs.copy(ibs, 4);
        ibs.get(4);   /* channel_config */
        bs.put(0, 4);
        bs.copy(ibs, 3);

        bs.put(0, 4); /* element_instance_tag */
        bs.put(1, 2); /* profile: LC */
        bs.put(sf_index, 4);
        bs.put(2, 4); /* num_front_channel_elements */
        bs.put(0, 4); /* num_side_channel_elements  */
        bs.put(2, 4); /* num_back_channel_elements  */
        bs.put(1, 2); /* num_lfe_channel_elements   */
        bs.put(0, 3); /* num_assoc_data_elements    */
        bs.put(0, 4); /* num_valid_cc_elements      */
        bs.put(0, 3); /* mono_mixdown, stereo_mixdown, matrix_mixdown */

        /* C */
        bs.put(0, 1); /* front_element_is_cpe */
        bs.put(0, 4); /* front_element_tag_select */
        /* L+R */
        bs.put(1, 1); /* front_element_is_cpe */
        bs.put(0, 4); /* front_element_tag_select */
        /* Ls+Rs */
        bs.put(1, 1); /* back_element_is_cpe */
        bs.put(1, 4); /* back_element_tag_select */
        /* Rls+Rrs */
        bs.put(1, 1); /* back_element_is_cpe */
        bs.put(2, 4); /* back_element_tag_select */
        /* LFE */
        bs.put(0, 4); /* lfe_elementtag_select */
        bs.byteAlign();
        bs.put(0, 8); /* comment_field_bytes*/

        size_t len = bs.position() / 8;
        std::vector<uint8_t> result(asc->size() - 2 + len);
        std::memcpy(result.data(), bs.data(), len);
        if (asc->size() > 2)
            std::memcpy(&result[len], asc->data() + 2, asc->size() - 2);
        asc->swap(result);
    }
}
