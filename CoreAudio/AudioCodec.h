#ifndef AudioCodec_H
#define AudioCodec_H

#include "CoreAudioTypes.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {
   kAudioCodecPropertyNameCFString                        = 'lnam',
   kAudioCodecPropertyManufacturerCFString                = 'lmak',
   kAudioCodecPropertyFormatCFString                      = 'lfor',
   //kAudioCodecPropertyHasVariablePacketByteSizes          = 'vpk?',
   kAudioCodecPropertySupportedInputFormats               = 'ifm#',
   kAudioCodecPropertySupportedOutputFormats              = 'ofm#',
   kAudioCodecPropertyAvailableInputSampleRates           = 'aisr',
   kAudioCodecPropertyAvailableOutputSampleRates          = 'aosr',
   kAudioCodecPropertyAvailableBitRateRange               = 'abrt',
   kAudioCodecPropertyMinimumNumberInputPackets           = 'mnip',
   kAudioCodecPropertyMinimumNumberOutputPackets          = 'mnop',
   kAudioCodecPropertyAvailableNumberChannels             = 'cmnc',
   kAudioCodecPropertyDoesSampleRateConversion            = 'lmrc',
   kAudioCodecPropertyAvailableInputChannelLayoutTags     = 'aicl',
   kAudioCodecPropertyAvailableOutputChannelLayoutTags    = 'aocl',
   kAudioCodecPropertyInputFormatsForOutputFormat         = 'if4o',
   kAudioCodecPropertyOutputFormatsForInputFormat         = 'of4i',
   kAudioCodecPropertyFormatInfo                          = 'acfi',
};

enum {
   kAudioCodecPropertyInputBufferSize               = 'tbuf',
   kAudioCodecPropertyPacketFrameSize               = 'pakf',
   kAudioCodecPropertyMaximumPacketByteSize         = 'pakb',
   kAudioCodecPropertyCurrentInputFormat            = 'ifmt',
   kAudioCodecPropertyCurrentOutputFormat           = 'ofmt',
   kAudioCodecPropertyMagicCookie                   = 'kuki',
   kAudioCodecPropertyUsedInputBufferSize           = 'ubuf',
   kAudioCodecPropertyIsInitialized                 = 'init',
   kAudioCodecPropertyCurrentTargetBitRate          = 'brat',
   kAudioCodecPropertyCurrentInputSampleRate        = 'cisr',
   kAudioCodecPropertyCurrentOutputSampleRate       = 'cosr',
   kAudioCodecPropertyQualitySetting                = 'srcq',
   kAudioCodecPropertyApplicableBitRateRange        = 'brta',
   kAudioCodecPropertyApplicableInputSampleRates    = 'isra',
   kAudioCodecPropertyApplicableOutputSampleRates   = 'osra',
   kAudioCodecPropertyPaddedZeros                   = 'pad0',
   kAudioCodecPropertyPrimeMethod                   = 'prmm',
   kAudioCodecPropertyPrimeInfo                     = 'prim',
   kAudioCodecPropertyCurrentInputChannelLayout     = 'icl ',
   kAudioCodecPropertyCurrentOutputChannelLayout    = 'ocl ',
   kAudioCodecPropertySettings                      = 'acs ',
   kAudioCodecPropertyFormatList                    = 'acfl',
   kAudioCodecPropertyBitRateControlMode            = 'acbf',
   kAudioCodecPropertySoundQualityForVBR            = 'vbrq',
   kAudioCodecPropertyMinimumDelayMode              = 'mdel'
};

enum {
   kAudioCodecBitRateControlMode_Constant                   = 0,
   kAudioCodecBitRateControlMode_LongTermAverage            = 1,
   kAudioCodecBitRateControlMode_VariableConstrained        = 2,
   kAudioCodecBitRateControlMode_Variable                   = 3,
};

typedef ComponentInstance    AudioCodec;
typedef UInt32 AudioCodecPropertyID;

typedef struct AudioCodecPrimeInfo {
   UInt32        leadingFrames;
   UInt32        trailingFrames;
} AudioCodecPrimeInfo;

ComponentResult AudioCodecGetProperty (
   AudioCodec inCodec,
   AudioCodecPropertyID inPropertyID,
   UInt32 *ioPropertyDataSize,
   void *outPropertyData);

ComponentResult AudioCodecGetPropertyInfo (
   AudioCodec inCodec,
   AudioCodecPropertyID inPropertyID,
   UInt32 *outSize,
   Boolean *outWritable);

ComponentResult AudioCodecInitialize (
   AudioCodec inCodec,
   const AudioStreamBasicDescription *inInputFormat,
   const AudioStreamBasicDescription *inOutputFormat,
   const void *inMagicCookie,
   UInt32 inMagicCookieByteSize);

ComponentResult AudioCodecSetProperty (
   AudioCodec inCodec,
   AudioCodecPropertyID inPropertyID,
   UInt32 inPropertyDataSize,
   const void *inPropertyData);

ComponentResult AudioCodecUninitialize(AudioCodec inCodec);

#ifdef __cplusplus
}
#endif

#endif
