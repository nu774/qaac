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

#ifndef REFALAC
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
}
