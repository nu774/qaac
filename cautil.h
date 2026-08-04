#ifndef CAUTIL_H
#define CAUTIL_H

#include <string>
#include <vector>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <stdint.h>
#include <utf8.h>
#ifdef __APPLE__
#include <CoreServices/CoreServices.h>
#include <AudioToolbox/AudioToolbox.h>
#else
#include "CoreAudio/CoreFoundation.h"
#include "CoreAudio/CoreAudioTypes.h"
#endif
#include "util.h"

typedef std::shared_ptr<const __CFString> CFStringPtr;
typedef std::shared_ptr<const __CFDictionary> CFDictionaryPtr;

class CoreAudioException: public std::runtime_error
{
    long m_error_code;
public:
    CoreAudioException(const std::string &s, long code)
        : std::runtime_error(s)
    {
        m_error_code = code;
    }
    long code() const { return m_error_code; }
    bool isNotSupportedError() const
    {
        return m_error_code == FOURCC('t','y','p','?') ||
               m_error_code == FOURCC('f','m','t','?') ||
               m_error_code == FOURCC('p','t','y','?') ||
               m_error_code == FOURCC('c','h','k','?');
    }
};

#define CHECKCA(expr) \
    do { \
        long err = expr; \
        if (err) { \
            std::string msg = cautil::make_coreaudio_error(err, #expr); \
            throw CoreAudioException(msg, err); \
        } \
    } while (0)

namespace cautil {
    std::string make_coreaudio_error(long code, const char *s);

#ifdef QAAC
    std::string CF2US(CFStringRef str);

    CFStringPtr US2CF(const std::string &s);
#endif

    inline size_t sizeofAudioChannelLayout(const AudioChannelLayout &acl)
    {
        int n = acl.mNumberChannelDescriptions;
        return offsetof(AudioChannelLayout, mChannelDescriptions[1])
                + std::max(0, n - 1) * sizeof(AudioChannelDescription);
    }
    inline int numChannelsOfAudioChannelLayout(const AudioChannelLayout *acl)
    {
        switch (acl->mChannelLayoutTag) {
        case kAudioChannelLayoutTag_UseChannelDescriptions:
            return acl->mNumberChannelDescriptions;
        case kAudioChannelLayoutTag_UseChannelBitmap:
            return util::bitcount(acl->mChannelBitmap);
        }
        return acl->mChannelLayoutTag & 0xffff;
    }
    AudioStreamBasicDescription
        buildASBDForPCM(double sample_rate, unsigned channels,
                        unsigned bits, unsigned type_flags,
                        unsigned alignment=0);

    AudioStreamBasicDescription
        buildASBDForPCM2(double sample_rate, unsigned channels,
                         unsigned valid_bits, unsigned pack_bits,
                         unsigned type_flags,
                         unsigned alignment=kAudioFormatFlagIsAlignedHigh);

    std::vector<uint8_t>
        parseMagicCookieAAC(const std::vector<uint8_t> &cookie);

    void replaceASCInMagicCookie(std::vector<uint8_t> *cookie,
                                 const std::vector<uint8_t> &data);

    void parseASC(const std::vector<uint8_t> &asc,
                  AudioStreamBasicDescription *asbd,
                  std::vector<uint32_t> *channels);

    void insert71RearPCEToASC(std::vector<uint8_t> *asc);
}

#endif
