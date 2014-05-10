#include "cautil.h"
#include "dl.h"

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
    void parseMagicCookieAAC(const std::vector<uint8_t> &cookie,
                             std::vector<uint8_t> *decSpecificConfig)
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
                    std::vector<uint8_t> vec(size);
                    std::memcpy(&vec[0], q, size);
                    decSpecificConfig->swap(vec);
                }
                return;
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
}
