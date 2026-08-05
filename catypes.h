#ifndef CATYPES_H
#define CATYPES_H

#include <stdint.h>

/*
 * Local mirrors of a few CoreAudioToolbox POD types, binary-compatible with
 * Apple's real definitions (see cautil.h for the conversion functions and
 * the static_asserts that guard this claim).
 *
 * Portable/core code (ISource, ISink, IEncoder and their implementations)
 * should use these ca:: types instead of the real CoreAudio types, so that
 * it never needs to #include either Apple's system headers or our local
 * Windows shim (the CoreAudio directory). Only code that actually calls
 * CoreAudio APIs is a real boundary and should convert to/from the native
 * type there.
 */
namespace ca {

struct AudioStreamBasicDescription {
    double   mSampleRate;
    uint32_t mFormatID;
    uint32_t mFormatFlags;
    uint32_t mBytesPerPacket;
    uint32_t mFramesPerPacket;
    uint32_t mBytesPerFrame;
    uint32_t mChannelsPerFrame;
    uint32_t mBitsPerChannel;
    uint32_t mReserved;
};

struct AudioFilePacketTableInfo {
    int64_t mNumberValidFrames;
    int32_t mPrimingFrames;
    int32_t mRemainderFrames;
};

struct AudioStreamPacketDescription {
    int64_t  mStartOffset;
    uint32_t mVariableFramesInPacket;
    uint32_t mDataByteSize;
};

/*
 * A handful of kAudioFormatFlag_* and kAudioChannelLayoutTag_* bit
 * values, needed by portable code that builds ca::AudioStreamBasicDescription
 * values or writes CAF-style headers without linking against any
 * CoreAudio header. These are part of Apple's long-frozen public C ABI,
 * so hardcoding them here carries no real drift risk.
 */
const uint32_t kAudioFormatFlagIsFloat = 1u << 0;
const uint32_t kAudioFormatFlagIsBigEndian = 1u << 1;
const uint32_t kAudioFormatFlagIsSignedInteger = 1u << 2;
const uint32_t kAudioFormatFlagIsPacked = 1u << 3;
const uint32_t kAudioFormatFlagIsAlignedHigh = 1u << 4;
const uint32_t kAudioChannelLayoutTag_UseChannelBitmap = (1u << 16) | 0;

} // namespace ca

#endif
