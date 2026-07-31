#if !defined(__CoreAudioTypes_h__)
#define __CoreAudioTypes_h__

#include "MacTypes.h"

struct AudioValueRange
{
    Float64 mMinimum;
    Float64 mMaximum;
};
typedef struct AudioValueRange  AudioValueRange;

struct AudioBuffer
{
    UInt32  mNumberChannels;
    UInt32  mDataByteSize;
    void*   mData;
};
typedef struct AudioBuffer  AudioBuffer;

struct AudioBufferList
{
    UInt32      mNumberBuffers;
    AudioBuffer mBuffers[kVariableLengthArray];
};
typedef struct AudioBufferList  AudioBufferList;

struct AudioStreamBasicDescription
{
    Float64 mSampleRate;
    UInt32  mFormatID;
    UInt32  mFormatFlags;
    UInt32  mBytesPerPacket;
    UInt32  mFramesPerPacket;
    UInt32  mBytesPerFrame;
    UInt32  mChannelsPerFrame;
    UInt32  mBitsPerChannel;
    UInt32  mReserved;
};
typedef struct AudioStreamBasicDescription  AudioStreamBasicDescription;

enum
{
    kAudioFormatFlagIsFloat                     = (1L << 0),
    kAudioFormatFlagIsBigEndian                 = (1L << 1),
    kAudioFormatFlagIsSignedInteger             = (1L << 2),
    kAudioFormatFlagIsPacked                    = (1L << 3),
    kAudioFormatFlagIsAlignedHigh               = (1L << 4),
    kAudioFormatFlagIsNonInterleaved            = (1L << 5),
    kAudioFormatFlagIsNonMixable                = (1L << 6),
    kAudioFormatFlagsAreAllClear                = (1L << 31),
    
    kLinearPCMFormatFlagIsFloat                 = kAudioFormatFlagIsFloat,
    kLinearPCMFormatFlagIsBigEndian             = kAudioFormatFlagIsBigEndian,
    kLinearPCMFormatFlagIsSignedInteger         = kAudioFormatFlagIsSignedInteger,
    kLinearPCMFormatFlagIsPacked                = kAudioFormatFlagIsPacked,
    kLinearPCMFormatFlagIsAlignedHigh           = kAudioFormatFlagIsAlignedHigh,
    kLinearPCMFormatFlagIsNonInterleaved        = kAudioFormatFlagIsNonInterleaved,
    kLinearPCMFormatFlagIsNonMixable            = kAudioFormatFlagIsNonMixable,
    kLinearPCMFormatFlagsAreAllClear            = kAudioFormatFlagsAreAllClear,
    
    kAppleLosslessFormatFlag_16BitSourceData    = 1,
    kAppleLosslessFormatFlag_20BitSourceData    = 2,
    kAppleLosslessFormatFlag_24BitSourceData    = 3,
    kAppleLosslessFormatFlag_32BitSourceData    = 4
};

struct  AudioStreamPacketDescription
{
    SInt64  mStartOffset;
    UInt32  mVariableFramesInPacket;
    UInt32  mDataByteSize;
};
typedef struct AudioStreamPacketDescription AudioStreamPacketDescription;

typedef UInt32 AudioChannelLabel;
typedef UInt32 AudioChannelLayoutTag;

struct AudioChannelDescription
{
    AudioChannelLabel   mChannelLabel;
    UInt32              mChannelFlags;
    Float32             mCoordinates[3];
};
typedef struct AudioChannelDescription AudioChannelDescription;

struct AudioChannelLayout
{
    AudioChannelLayoutTag       mChannelLayoutTag;
    UInt32                      mChannelBitmap;
    UInt32                      mNumberChannelDescriptions;
    AudioChannelDescription     mChannelDescriptions[kVariableLengthArray];
};
typedef struct AudioChannelLayout AudioChannelLayout;

#define kAudioChannelLabel_Unknown 0xFFFFFFFF,   // unknown or unspecified other use
enum
{
    kAudioChannelLabel_Unused                   = 0,            // channel is present, but has no intended use or destination
    kAudioChannelLabel_UseCoordinates           = 100,          // channel is described by the mCoordinates fields.

    kAudioChannelLabel_Left                     = 1,
    kAudioChannelLabel_Right                    = 2,
    kAudioChannelLabel_Center                   = 3,
    kAudioChannelLabel_LFEScreen                = 4,
    kAudioChannelLabel_LeftSurround             = 5,            // WAVE: "Back Left"
    kAudioChannelLabel_RightSurround            = 6,            // WAVE: "Back Right"
    kAudioChannelLabel_LeftCenter               = 7,
    kAudioChannelLabel_RightCenter              = 8,
    kAudioChannelLabel_CenterSurround           = 9,            // WAVE: "Back Center" or plain "Rear Surround"
    kAudioChannelLabel_LeftSurroundDirect       = 10,           // WAVE: "Side Left"
    kAudioChannelLabel_RightSurroundDirect      = 11,           // WAVE: "Side Right"
    kAudioChannelLabel_TopCenterSurround        = 12,
    kAudioChannelLabel_VerticalHeightLeft       = 13,           // WAVE: "Top Front Left"
    kAudioChannelLabel_VerticalHeightCenter     = 14,           // WAVE: "Top Front Center"
    kAudioChannelLabel_VerticalHeightRight      = 15,           // WAVE: "Top Front Right"

    kAudioChannelLabel_TopBackLeft              = 16,
    kAudioChannelLabel_TopBackCenter            = 17,
    kAudioChannelLabel_TopBackRight             = 18,

    kAudioChannelLabel_RearSurroundLeft         = 33,
    kAudioChannelLabel_RearSurroundRight        = 34,
    kAudioChannelLabel_LeftWide                 = 35,
    kAudioChannelLabel_RightWide                = 36,
    kAudioChannelLabel_LFE2                     = 37,
    kAudioChannelLabel_LeftTotal                = 38,           // matrix encoded 4 channels
    kAudioChannelLabel_RightTotal               = 39,           // matrix encoded 4 channels
    kAudioChannelLabel_HearingImpaired          = 40,
    kAudioChannelLabel_Narration                = 41,
    kAudioChannelLabel_Mono                     = 42,
    kAudioChannelLabel_DialogCentricMix         = 43,

    kAudioChannelLabel_CenterSurroundDirect     = 44,           // back center, non diffuse
    
    kAudioChannelLabel_Haptic                   = 45,

    // first order ambisonic channels
    kAudioChannelLabel_Ambisonic_W              = 200,
    kAudioChannelLabel_Ambisonic_X              = 201,
    kAudioChannelLabel_Ambisonic_Y              = 202,
    kAudioChannelLabel_Ambisonic_Z              = 203,

    // Mid/Side Recording
    kAudioChannelLabel_MS_Mid                   = 204,
    kAudioChannelLabel_MS_Side                  = 205,

    // X-Y Recording
    kAudioChannelLabel_XY_X                     = 206,
    kAudioChannelLabel_XY_Y                     = 207,

    // other
    kAudioChannelLabel_HeadphonesLeft           = 301,
    kAudioChannelLabel_HeadphonesRight          = 302,
    kAudioChannelLabel_ClickTrack               = 304,
    kAudioChannelLabel_ForeignLanguage          = 305,

    // generic discrete channel
    kAudioChannelLabel_Discrete                 = 400,

    // numbered discrete channel
    kAudioChannelLabel_Discrete_0               = (1L<<16) | 0,
    kAudioChannelLabel_Discrete_1               = (1L<<16) | 1,
    kAudioChannelLabel_Discrete_2               = (1L<<16) | 2,
    kAudioChannelLabel_Discrete_3               = (1L<<16) | 3,
    kAudioChannelLabel_Discrete_4               = (1L<<16) | 4,
    kAudioChannelLabel_Discrete_5               = (1L<<16) | 5,
    kAudioChannelLabel_Discrete_6               = (1L<<16) | 6,
    kAudioChannelLabel_Discrete_7               = (1L<<16) | 7,
    kAudioChannelLabel_Discrete_8               = (1L<<16) | 8,
    kAudioChannelLabel_Discrete_9               = (1L<<16) | 9,
    kAudioChannelLabel_Discrete_10              = (1L<<16) | 10,
    kAudioChannelLabel_Discrete_11              = (1L<<16) | 11,
    kAudioChannelLabel_Discrete_12              = (1L<<16) | 12,
    kAudioChannelLabel_Discrete_13              = (1L<<16) | 13,
    kAudioChannelLabel_Discrete_14              = (1L<<16) | 14,
    kAudioChannelLabel_Discrete_15              = (1L<<16) | 15,
    kAudioChannelLabel_Discrete_65535           = (1L<<16) | 65535
};

#define AudioChannelLayoutTag_GetNumberOfChannels(layoutTag) ((UInt32)((layoutTag) & 0x0000FFFF))

#define kAudioChannelLayoutTag_Unknown 0xFFFF0000
enum
{
    kAudioChannelLayoutTag_UseChannelDescriptions   = (0L<<16) | 0,     // use the array of AudioChannelDescriptions to define the mapping.
    kAudioChannelLayoutTag_UseChannelBitmap         = (1L<<16) | 0,     // use the bitmap to define the mapping.

    kAudioChannelLayoutTag_Mono                     = (100L<<16) | 1,   // a standard mono stream
    kAudioChannelLayoutTag_Stereo                   = (101L<<16) | 2,   // a standard stereo stream (L R) - implied playback
    kAudioChannelLayoutTag_StereoHeadphones         = (102L<<16) | 2,   // a standard stereo stream (L R) - implied headphone playbac
    kAudioChannelLayoutTag_MatrixStereo             = (103L<<16) | 2,   // a matrix encoded stereo stream (Lt, Rt)
    kAudioChannelLayoutTag_MidSide                  = (104L<<16) | 2,   // mid/side recording
    kAudioChannelLayoutTag_XY                       = (105L<<16) | 2,   // coincident mic pair (often 2 figure 8's)
    kAudioChannelLayoutTag_Binaural                 = (106L<<16) | 2,   // binaural stereo (left, right)
    kAudioChannelLayoutTag_Ambisonic_B_Format       = (107L<<16) | 4,   // W, X, Y, Z

    kAudioChannelLayoutTag_Quadraphonic             = (108L<<16) | 4,   // front left, front right, back left, back right

    kAudioChannelLayoutTag_Pentagonal               = (109L<<16) | 5,   // left, right, rear left, rear right, center

    kAudioChannelLayoutTag_Hexagonal                = (110L<<16) | 6,   // left, right, rear left, rear right, center, rear

    kAudioChannelLayoutTag_Octagonal                = (111L<<16) | 8,   // front left, front right, rear left, rear right,
                                                                        // front center, rear center, side left, side right

    kAudioChannelLayoutTag_Cube                     = (112L<<16) | 8,   // left, right, rear left, rear right
                                                                        // top left, top right, top rear left, top rear right

    //  MPEG defined layouts
    kAudioChannelLayoutTag_MPEG_1_0                 = kAudioChannelLayoutTag_Mono,          //  C
    kAudioChannelLayoutTag_MPEG_2_0                 = kAudioChannelLayoutTag_Stereo,        //  L R
    kAudioChannelLayoutTag_MPEG_3_0_A               = (113L<<16) | 3,                       //  L R C
    kAudioChannelLayoutTag_MPEG_3_0_B               = (114L<<16) | 3,                       //  C L R
    kAudioChannelLayoutTag_MPEG_4_0_A               = (115L<<16) | 4,                       //  L R C Cs
    kAudioChannelLayoutTag_MPEG_4_0_B               = (116L<<16) | 4,                       //  C L R Cs
    kAudioChannelLayoutTag_MPEG_5_0_A               = (117L<<16) | 5,                       //  L R C Ls Rs
    kAudioChannelLayoutTag_MPEG_5_0_B               = (118L<<16) | 5,                       //  L R Ls Rs C
    kAudioChannelLayoutTag_MPEG_5_0_C               = (119L<<16) | 5,                       //  L C R Ls Rs
    kAudioChannelLayoutTag_MPEG_5_0_D               = (120L<<16) | 5,                       //  C L R Ls Rs
    kAudioChannelLayoutTag_MPEG_5_1_A               = (121L<<16) | 6,                       //  L R C LFE Ls Rs
    kAudioChannelLayoutTag_MPEG_5_1_B               = (122L<<16) | 6,                       //  L R Ls Rs C LFE
    kAudioChannelLayoutTag_MPEG_5_1_C               = (123L<<16) | 6,                       //  L C R Ls Rs LFE
    kAudioChannelLayoutTag_MPEG_5_1_D               = (124L<<16) | 6,                       //  C L R Ls Rs LFE
    kAudioChannelLayoutTag_MPEG_6_1_A               = (125L<<16) | 7,                       //  L R C LFE Ls Rs Cs
    kAudioChannelLayoutTag_MPEG_7_1_A               = (126L<<16) | 8,                       //  L R C LFE Ls Rs Lc Rc
    kAudioChannelLayoutTag_MPEG_7_1_B               = (127L<<16) | 8,                       //  C Lc Rc L R Ls Rs LFE    (doc: IS-13818-7 MPEG2-AAC Table 3.1)
    kAudioChannelLayoutTag_MPEG_7_1_C               = (128L<<16) | 8,                       //  L R C LFE Ls Rs Rls Rrs
    kAudioChannelLayoutTag_Emagic_Default_7_1       = (129L<<16) | 8,                       //  L R Ls Rs C LFE Lc Rc
    kAudioChannelLayoutTag_SMPTE_DTV                = (130L<<16) | 8,                       //  L R C LFE Ls Rs Lt Rt
                                                                                            //      (kAudioChannelLayoutTag_ITU_5_1 plus a matrix encoded stereo mix)

    //  ITU defined layouts
    kAudioChannelLayoutTag_ITU_1_0                  = kAudioChannelLayoutTag_Mono,          //  C
    kAudioChannelLayoutTag_ITU_2_0                  = kAudioChannelLayoutTag_Stereo,        //  L R

    kAudioChannelLayoutTag_ITU_2_1                  = (131L<<16) | 3,                       //  L R Cs
    kAudioChannelLayoutTag_ITU_2_2                  = (132L<<16) | 4,                       //  L R Ls Rs
    kAudioChannelLayoutTag_ITU_3_0                  = kAudioChannelLayoutTag_MPEG_3_0_A,    //  L R C
    kAudioChannelLayoutTag_ITU_3_1                  = kAudioChannelLayoutTag_MPEG_4_0_A,    //  L R C Cs

    kAudioChannelLayoutTag_ITU_3_2                  = kAudioChannelLayoutTag_MPEG_5_0_A,    //  L R C Ls Rs
    kAudioChannelLayoutTag_ITU_3_2_1                = kAudioChannelLayoutTag_MPEG_5_1_A,    //  L R C LFE Ls Rs
    kAudioChannelLayoutTag_ITU_3_4_1                = kAudioChannelLayoutTag_MPEG_7_1_C,    //  L R C LFE Ls Rs Rls Rrs

    // DVD defined layouts
    kAudioChannelLayoutTag_DVD_0                    = kAudioChannelLayoutTag_Mono,          // C (mono)
    kAudioChannelLayoutTag_DVD_1                    = kAudioChannelLayoutTag_Stereo,        // L R
    kAudioChannelLayoutTag_DVD_2                    = kAudioChannelLayoutTag_ITU_2_1,       // L R Cs
    kAudioChannelLayoutTag_DVD_3                    = kAudioChannelLayoutTag_ITU_2_2,       // L R Ls Rs
    kAudioChannelLayoutTag_DVD_4                    = (133L<<16) | 3,                       // L R LFE
    kAudioChannelLayoutTag_DVD_5                    = (134L<<16) | 4,                       // L R LFE Cs
    kAudioChannelLayoutTag_DVD_6                    = (135L<<16) | 5,                       // L R LFE Ls Rs
    kAudioChannelLayoutTag_DVD_7                    = kAudioChannelLayoutTag_MPEG_3_0_A,    // L R C
    kAudioChannelLayoutTag_DVD_8                    = kAudioChannelLayoutTag_MPEG_4_0_A,    // L R C Cs
    kAudioChannelLayoutTag_DVD_9                    = kAudioChannelLayoutTag_MPEG_5_0_A,    // L R C Ls Rs
    kAudioChannelLayoutTag_DVD_10                   = (136L<<16) | 4,                       // L R C LFE
    kAudioChannelLayoutTag_DVD_11                   = (137L<<16) | 5,                       // L R C LFE Cs
    kAudioChannelLayoutTag_DVD_12                   = kAudioChannelLayoutTag_MPEG_5_1_A,    // L R C LFE Ls Rs
    // 13 through 17 are duplicates of 8 through 12.
    kAudioChannelLayoutTag_DVD_13                   = kAudioChannelLayoutTag_DVD_8,         // L R C Cs
    kAudioChannelLayoutTag_DVD_14                   = kAudioChannelLayoutTag_DVD_9,         // L R C Ls Rs
    kAudioChannelLayoutTag_DVD_15                   = kAudioChannelLayoutTag_DVD_10,        // L R C LFE
    kAudioChannelLayoutTag_DVD_16                   = kAudioChannelLayoutTag_DVD_11,        // L R C LFE Cs
    kAudioChannelLayoutTag_DVD_17                   = kAudioChannelLayoutTag_DVD_12,        // L R C LFE Ls Rs
    kAudioChannelLayoutTag_DVD_18                   = (138L<<16) | 5,                       // L R Ls Rs LFE
    kAudioChannelLayoutTag_DVD_19                   = kAudioChannelLayoutTag_MPEG_5_0_B,    // L R Ls Rs C
    kAudioChannelLayoutTag_DVD_20                   = kAudioChannelLayoutTag_MPEG_5_1_B,    // L R Ls Rs C LFE

    // These layouts are recommended for AudioUnit usage
        // These are the symmetrical layouts
    kAudioChannelLayoutTag_AudioUnit_4              = kAudioChannelLayoutTag_Quadraphonic,
    kAudioChannelLayoutTag_AudioUnit_5              = kAudioChannelLayoutTag_Pentagonal,
    kAudioChannelLayoutTag_AudioUnit_6              = kAudioChannelLayoutTag_Hexagonal,
    kAudioChannelLayoutTag_AudioUnit_8              = kAudioChannelLayoutTag_Octagonal,
        // These are the surround-based layouts
    kAudioChannelLayoutTag_AudioUnit_5_0            = kAudioChannelLayoutTag_MPEG_5_0_B,    // L R Ls Rs C
    kAudioChannelLayoutTag_AudioUnit_6_0            = (139L<<16) | 6,                       // L R Ls Rs C Cs
    kAudioChannelLayoutTag_AudioUnit_7_0            = (140L<<16) | 7,                       // L R Ls Rs C Rls Rrs
    kAudioChannelLayoutTag_AudioUnit_5_1            = kAudioChannelLayoutTag_MPEG_5_1_A,    // L R C LFE Ls Rs
    kAudioChannelLayoutTag_AudioUnit_6_1            = kAudioChannelLayoutTag_MPEG_6_1_A,    // L R C LFE Ls Rs Cs
    kAudioChannelLayoutTag_AudioUnit_7_1            = kAudioChannelLayoutTag_MPEG_7_1_C,    // L R C LFE Ls Rs Rls Rrs
    kAudioChannelLayoutTag_AudioUnit_7_1_Front      = kAudioChannelLayoutTag_MPEG_7_1_A,    // L R C LFE Ls Rs Lc Rc

    kAudioChannelLayoutTag_AAC_3_0                  = kAudioChannelLayoutTag_MPEG_3_0_B,    // C L R
    kAudioChannelLayoutTag_AAC_Quadraphonic         = kAudioChannelLayoutTag_Quadraphonic,  // L R Ls Rs
    kAudioChannelLayoutTag_AAC_4_0                  = kAudioChannelLayoutTag_MPEG_4_0_B,    // C L R Cs
    kAudioChannelLayoutTag_AAC_5_0                  = kAudioChannelLayoutTag_MPEG_5_0_D,    // C L R Ls Rs
    kAudioChannelLayoutTag_AAC_5_1                  = kAudioChannelLayoutTag_MPEG_5_1_D,    // C L R Ls Rs Lfe
    kAudioChannelLayoutTag_AAC_6_0                  = (141L<<16) | 6,                       // C L R Ls Rs Cs
    kAudioChannelLayoutTag_AAC_6_1                  = (142L<<16) | 7,                       // C L R Ls Rs Cs Lfe
    kAudioChannelLayoutTag_AAC_7_0                  = (143L<<16) | 7,                       // C L R Ls Rs Rls Rrs
    kAudioChannelLayoutTag_AAC_7_1                  = kAudioChannelLayoutTag_MPEG_7_1_B,    // C Lc Rc L R Ls Rs Lfe
    kAudioChannelLayoutTag_AAC_Octagonal            = (144L<<16) | 8,                       // C L R Ls Rs Rls Rrs Cs

    kAudioChannelLayoutTag_TMH_10_2_std             = (145L<<16) | 16,                      // L R C Vhc Lsd Rsd Ls Rs Vhl Vhr Lw Rw Csd Cs LFE1 LFE2
    kAudioChannelLayoutTag_TMH_10_2_full            = (146L<<16) | 21,                      // TMH_10_2_std plus: Lc Rc HI VI Haptic

    kAudioChannelLayoutTag_DiscreteInOrder          = (147L<<16) | 0,                       // needs to be ORed with the actual number of channels  

    kAudioChannelLayoutTag_AudioUnit_7_0_Front      = (148L<<16) | 7,                       // L R Ls Rs C Lc Rc

    kAudioChannelLayoutTag_AC3_1_0_1                = (149L<<16) | 2,                       // C LFE
    kAudioChannelLayoutTag_AC3_3_0                  = (150L<<16) | 3,                       // L C R
    kAudioChannelLayoutTag_AC3_3_1                  = (151L<<16) | 4,                       // L C R Cs
    kAudioChannelLayoutTag_AC3_3_0_1                = (152L<<16) | 4,                       // L C R LFE
    kAudioChannelLayoutTag_AC3_2_1_1                = (153L<<16) | 4,                       // L R Cs LFE
    kAudioChannelLayoutTag_AC3_3_1_1                = (154L<<16) | 5,                       // L C R Cs LFE

    kAudioChannelLayoutTag_EAC_6_0_A                = (155<<16) | 6,                        // L C R Ls Rs Cs
    kAudioChannelLayoutTag_EAC_7_0_A                = (156<<16) | 7,                        // L C R Ls Rs Rls Rrs

    kAudioChannelLayoutTag_EAC3_6_1_A               = (157<<16) | 7,                        // L C R Ls Rs LFE Cs
    kAudioChannelLayoutTag_EAC3_6_1_B               = (158<<16) | 7,                        // L C R Ls Rs LFE Ts
    kAudioChannelLayoutTag_EAC3_6_1_C               = (159<<16) | 7,                        // L C R Ls Rs LFE Vhc
    kAudioChannelLayoutTag_EAC3_7_1_A               = (160<<16) | 8,                        // L C R Ls Rs LFE Rls Rrs
    kAudioChannelLayoutTag_EAC3_7_1_B               = (161<<16) | 8,                        // L C R Ls Rs LFE Lc Rc
    kAudioChannelLayoutTag_EAC3_7_1_C               = (162<<16) | 8,                        // L C R Ls Rs LFE Lsd Rsd
    kAudioChannelLayoutTag_EAC3_7_1_D               = (163<<16) | 8,                        // L C R Ls Rs LFE Lw Rw
    kAudioChannelLayoutTag_EAC3_7_1_E               = (164<<16) | 8,                        // L C R Ls Rs LFE Vhl Vhr

    kAudioChannelLayoutTag_EAC3_7_1_F               = (165<<16) | 8,                        // L C R Ls Rs LFE Cs Ts
    kAudioChannelLayoutTag_EAC3_7_1_G               = (166<<16) | 8,                        // L C R Ls Rs LFE Cs Vhc
    kAudioChannelLayoutTag_EAC3_7_1_H               = (167<<16) | 8,                        // L C R Ls Rs LFE Ts Vhc

    kAudioChannelLayoutTag_DTS_3_1                  = (168<<16) | 4,                        // C L R LFE
    kAudioChannelLayoutTag_DTS_4_1                  = (169<<16) | 5,                        // C L R Cs LFE
    kAudioChannelLayoutTag_DTS_6_0_A                = (170<<16) | 6,                        // Lc Rc L R Ls Rs
    kAudioChannelLayoutTag_DTS_6_0_B                = (171<<16) | 6,                        // C L R Rls Rrs Ts
    kAudioChannelLayoutTag_DTS_6_0_C                = (172<<16) | 6,                        // C Cs L R Rls Rrs
    kAudioChannelLayoutTag_DTS_6_1_A                = (173<<16) | 7,                        // Lc Rc L R Ls Rs LFE
    kAudioChannelLayoutTag_DTS_6_1_B                = (174<<16) | 7,                        // C L R Rls Rrs Ts LFE
    kAudioChannelLayoutTag_DTS_6_1_C                = (175<<16) | 7,                        // C Cs L R Rls Rrs LFE
    kAudioChannelLayoutTag_DTS_7_0                  = (176<<16) | 7,                        // Lc C Rc L R Ls Rs
    kAudioChannelLayoutTag_DTS_7_1                  = (177<<16) | 8,                        // Lc C Rc L R Ls Rs LFE    
    kAudioChannelLayoutTag_DTS_8_0_A                = (178<<16) | 8,                        // Lc Rc L R Ls Rs Rls Rrs
    kAudioChannelLayoutTag_DTS_8_0_B                = (179<<16) | 8,                        // Lc C Rc L R Ls Cs Rs
    kAudioChannelLayoutTag_DTS_8_1_A                = (180<<16) | 9,                        // Lc Rc L R Ls Rs Rls Rrs LFE
    kAudioChannelLayoutTag_DTS_8_1_B                = (181<<16) | 9,                        // Lc C Rc L R Ls Cs Rs LFE
    kAudioChannelLayoutTag_DTS_6_1_D                = (182<<16) | 7,                        // C L R Ls Rs LFE Cs

    kAudioChannelLayoutTag_AAC_7_1_B                = (183<<16) | 8,                        // C L R Ls Rs Rls Rrs LFE
    kAudioChannelLayoutTag_AAC_7_1_C                = (184<<16) | 8,                        // C L R Ls Rs LFE Vhl Vhr

    kAudioChannelLayoutTag_WAVE_2_1                 = kAudioChannelLayoutTag_DVD_4,         // 3 channels, L R LFE
    kAudioChannelLayoutTag_WAVE_3_0                 = kAudioChannelLayoutTag_MPEG_3_0_A,    // 3 channels, L R C
    kAudioChannelLayoutTag_WAVE_4_0_A               = kAudioChannelLayoutTag_ITU_2_2,       // 4 channels, L R Ls Rs
    kAudioChannelLayoutTag_WAVE_4_0_B               = (185<<16) | 4,                        // L R Rls Rrs
    kAudioChannelLayoutTag_WAVE_5_0_A               = kAudioChannelLayoutTag_MPEG_5_0_A,    // 5 channels, L R C Ls Rs
    kAudioChannelLayoutTag_WAVE_5_0_B               = (186<<16) | 5,                        // L R C Rls Rrs
    kAudioChannelLayoutTag_WAVE_5_1_A               = kAudioChannelLayoutTag_MPEG_5_1_A,    // 6 channels, L R C LFE Ls Rs
    kAudioChannelLayoutTag_WAVE_5_1_B               = (187<<16) | 6,                        // L R C LFE Rls Rrs
    kAudioChannelLayoutTag_WAVE_6_1                 = (188<<16) | 7,                        // L R C LFE Cs Ls Rs
    kAudioChannelLayoutTag_WAVE_7_1                 = (189<<16) | 8,                        // L R C LFE Rls Rrs Ls Rs

    kAudioChannelLayoutTag_HOA_ACN_SN3D             = (190<<16) | 0,                        // Higher Order Ambisonics, Ambisonics Channel Number, SN3D normalization
                                                                                            // needs to be ORed with the actual number of channels (not the HOA order)
    kAudioChannelLayoutTag_HOA_ACN_N3D              = (191<<16) | 0,                        // Higher Order Ambisonics, Ambisonics Channel Number, N3D normalization
                                                                                            // needs to be ORed with the actual number of channels (not the HOA order)

    kAudioChannelLayoutTag_Atmos_5_1_2              = (194<<16) | 8,                        // L R C LFE Ls Rs Ltm Rtm
    kAudioChannelLayoutTag_Atmos_5_1_4              = (195<<16) | 10,                       // L R C LFE Ls Rs Vhl Vhr Ltr Rtr
    kAudioChannelLayoutTag_Atmos_7_1_2              = (196<<16) | 10,                       // L R C LFE Ls Rs Rls Rrs Ltm Rtm
    kAudioChannelLayoutTag_Atmos_7_1_4              = (192<<16) | 12,                       // L R C LFE Ls Rs Rls Rrs Vhl Vhr Ltr Rtr
    kAudioChannelLayoutTag_Atmos_9_1_6              = (193<<16) | 16,                       // L R C LFE Ls Rs Rls Rrs Lw Rw Vhl Vhr Ltm Rtm Ltr Rtr

    kAudioChannelLayoutTag_Logic_Mono               = kAudioChannelLayoutTag_Mono,          // C
    kAudioChannelLayoutTag_Logic_Stereo             = kAudioChannelLayoutTag_Stereo,        // L R
    kAudioChannelLayoutTag_Logic_Quadraphonic       = kAudioChannelLayoutTag_Quadraphonic,  // L R Ls Rs
    kAudioChannelLayoutTag_Logic_4_0_A              = kAudioChannelLayoutTag_MPEG_4_0_A,    // L R C Cs
    kAudioChannelLayoutTag_Logic_4_0_B              = kAudioChannelLayoutTag_MPEG_4_0_B,    // C L R Cs
    kAudioChannelLayoutTag_Logic_4_0_C              = (197U<<16) | 4,                       // L R Cs C
    kAudioChannelLayoutTag_Logic_5_0_A              = kAudioChannelLayoutTag_MPEG_5_0_A,    // L R C Ls Rs
    kAudioChannelLayoutTag_Logic_5_0_B              = kAudioChannelLayoutTag_MPEG_5_0_B,    // L R Ls Rs C
    kAudioChannelLayoutTag_Logic_5_0_C              = kAudioChannelLayoutTag_MPEG_5_0_C,    // L C R Ls Rs
    kAudioChannelLayoutTag_Logic_5_0_D              = kAudioChannelLayoutTag_MPEG_5_0_D,    // C L R Ls Rs
    kAudioChannelLayoutTag_Logic_5_1_A              = kAudioChannelLayoutTag_MPEG_5_1_A,    // L R C LFE Ls Rs
    kAudioChannelLayoutTag_Logic_5_1_B              = kAudioChannelLayoutTag_MPEG_5_1_B,    // L R Ls Rs C LFE
    kAudioChannelLayoutTag_Logic_5_1_C              = kAudioChannelLayoutTag_MPEG_5_1_C,    // L C R Ls Rs LFE
    kAudioChannelLayoutTag_Logic_5_1_D              = kAudioChannelLayoutTag_MPEG_5_1_D,    // C L R Ls Rs LFE
    kAudioChannelLayoutTag_Logic_6_0_A              = kAudioChannelLayoutTag_AAC_6_0,       // C L R Ls Rs Cs
    kAudioChannelLayoutTag_Logic_6_0_B              = (198U<<16) | 6,                       // L R Ls Rs Cs C
    kAudioChannelLayoutTag_Logic_6_0_C              = kAudioChannelLayoutTag_AudioUnit_6_0, // L R Ls Rs C Cs
    kAudioChannelLayoutTag_Logic_6_1_A              = kAudioChannelLayoutTag_AAC_6_1,       // C L R Ls Rs Cs LFE
    kAudioChannelLayoutTag_Logic_6_1_B              = (199U<<16) | 7,                       // L R Ls Rs Cs C LFE
    kAudioChannelLayoutTag_Logic_6_1_C              = kAudioChannelLayoutTag_MPEG_6_1_A,    // L R C LFE Ls Rs Cs
    kAudioChannelLayoutTag_Logic_6_1_D              = (200U<<16) | 7,                       // L C R Ls Cs Rs LFE
    kAudioChannelLayoutTag_Logic_7_1_A              = kAudioChannelLayoutTag_AudioUnit_7_1, // L R C LFE Ls Rs Rls Rrs
    kAudioChannelLayoutTag_Logic_7_1_B              = (201U<<16) | 8,                       // L R Ls Rs Rls Rrs C LFE
    kAudioChannelLayoutTag_Logic_7_1_C              = kAudioChannelLayoutTag_MPEG_7_1_C,    // L R C LFE Ls Rs Rls Rrs
    kAudioChannelLayoutTag_Logic_7_1_SDDS_A         = kAudioChannelLayoutTag_MPEG_7_1_A,    // L R C LFE Ls Rs Lc Rc
    kAudioChannelLayoutTag_Logic_7_1_SDDS_B         = kAudioChannelLayoutTag_MPEG_7_1_B,    // C Lc Rc L R Ls Rs LFE
    kAudioChannelLayoutTag_Logic_7_1_SDDS_C         = kAudioChannelLayoutTag_Emagic_Default_7_1, // L R Ls Rs C LFE Lc Rc
    kAudioChannelLayoutTag_Logic_Atmos_5_1_2        = kAudioChannelLayoutTag_Atmos_5_1_2,   // L R C LFE Ls Rs Ltm Rtm
    kAudioChannelLayoutTag_Logic_Atmos_5_1_4        = kAudioChannelLayoutTag_Atmos_5_1_4,   // L R C LFE Ls Rs Vhl Vhr Ltr Rtr
    kAudioChannelLayoutTag_Logic_Atmos_7_1_2        = kAudioChannelLayoutTag_Atmos_7_1_2,   // L R C LFE Ls Rs Rls Rrs Ltm Rtm
    kAudioChannelLayoutTag_Logic_Atmos_7_1_4_A      = kAudioChannelLayoutTag_Atmos_7_1_4,   // L R C LFE Ls Rs Rls Rrs Vhl Vhr Ltr Rtr
    kAudioChannelLayoutTag_Logic_Atmos_7_1_4_B      = (202U<<16) | 12,                      // L R Rls Rrs Ls Rs C LFE Vhl Vhr Ltr Rtr
    kAudioChannelLayoutTag_Logic_Atmos_7_1_6        = (203U<<16) | 14,                      // L R Rls Rrs Ls Rs C LFE Vhl Vhr Ltm Rtm Ltr Rtr

    // ISO/IEC 23091-3, channels w/orderings
    kAudioChannelLayoutTag_CICP_1                    = kAudioChannelLayoutTag_MPEG_1_0,      // C
    kAudioChannelLayoutTag_CICP_2                    = kAudioChannelLayoutTag_MPEG_2_0,      // L R
    kAudioChannelLayoutTag_CICP_3                    = kAudioChannelLayoutTag_MPEG_3_0_A,    // L R C
    kAudioChannelLayoutTag_CICP_4                    = kAudioChannelLayoutTag_MPEG_4_0_A,    // L R C Cs
    kAudioChannelLayoutTag_CICP_5                    = kAudioChannelLayoutTag_MPEG_5_0_A,    // L R C Ls Rs
    kAudioChannelLayoutTag_CICP_6                    = kAudioChannelLayoutTag_MPEG_5_1_A,    // L R C LFE Ls Rs
    kAudioChannelLayoutTag_CICP_7                    = kAudioChannelLayoutTag_MPEG_7_1_B,    // L R C LFE Ls Rs Lc Rc

    kAudioChannelLayoutTag_CICP_9                    = kAudioChannelLayoutTag_ITU_2_1,       // L R Cs
    kAudioChannelLayoutTag_CICP_10                   = kAudioChannelLayoutTag_ITU_2_2,       // L R Ls Rs
    kAudioChannelLayoutTag_CICP_11                   = kAudioChannelLayoutTag_MPEG_6_1_A,    // L R C LFE Ls Rs Cs
    kAudioChannelLayoutTag_CICP_12                   = kAudioChannelLayoutTag_MPEG_7_1_C,    // L R C LFE Ls Rs Rls Rrs
    kAudioChannelLayoutTag_CICP_13                   = (204U<<16) | 24,                      // Lc Rc C LFE2 Rls Rrs L R Cs LFE3 Lss Rss Vhl Vhr Vhc Ts Ltr Rtr Ltm Rtm Ctr Cb Lb Rb

    kAudioChannelLayoutTag_CICP_14                   = (205U<<16) | 8,                       // L R C LFE Ls Rs Vhl Vhr
    kAudioChannelLayoutTag_CICP_15                   = (206U<<16) | 12,                      // L R C LFE2 Rls Rrs LFE3 Lss Rss Vhl Vhr Ctr

    kAudioChannelLayoutTag_CICP_16                   = (207U<<16) | 10,                      // L R C LFE Ls Rs Vhl Vhr Lts Rts
    kAudioChannelLayoutTag_CICP_17                   = (208U<<16) | 12,                      // L R C LFE Ls Rs Vhl Vhr Vhc Lts Rts Ts
    kAudioChannelLayoutTag_CICP_18                   = (209U<<16) | 14,                      // L R C LFE Ls Rs Lbs Rbs Vhl Vhr Vhc Lts Rts Ts

    kAudioChannelLayoutTag_CICP_19                   = (210U<<16) | 12,                      // L R C LFE Rls Rrs Lss Rss Vhl Vhr Ltr Rtr
    kAudioChannelLayoutTag_CICP_20                   = (211U<<16) | 14,                      // L R C LFE Rls Rrs Lss Rss Vhl Vhr Ltr Rtr Leos Reos

    kAudioChannelLayoutTag_Ogg_3_0                   = kAudioChannelLayoutTag_AC3_3_0,        // 3 channels, L C R
    kAudioChannelLayoutTag_Ogg_4_0                   = kAudioChannelLayoutTag_WAVE_4_0_B,     // 4 channels, L R Rls Rrs
    kAudioChannelLayoutTag_Ogg_5_0                   = (212U<<16) | 5,                        // 5 channels, L C R Rls Rrs
    kAudioChannelLayoutTag_Ogg_5_1                   = (213U<<16) | 6,                        // 6 channels, L C R Rls Rrs LFE
    kAudioChannelLayoutTag_Ogg_6_1                   = (214U<<16) | 7,                        // 7 channels, L C R Ls Rs Cs LFE
    kAudioChannelLayoutTag_Ogg_7_1                   = (215U<<16) | 8,                        // 8 channels, L C R Ls Rs Rls Rrs LFE

    kAudioChannelLayoutTag_MPEG_5_0_E                = (216U<<16) | 5,                        // 5 channels, L R Rls Rrs C
    kAudioChannelLayoutTag_MPEG_5_1_E                = (217U<<16) | 6,                        // 6 channels, L R Rls Rrs C LFE
    kAudioChannelLayoutTag_MPEG_6_1_B                = (218U<<16) | 7,                        // 7 channels, L R Ls Rs C Cs LFE
    kAudioChannelLayoutTag_MPEG_7_1_D                = (219U<<16) | 8,                        // 8 channels, L R Rls Rrs Ls Rs C LFE

    kAudioChannelLayoutTag_BeginReserved             = 0xF0000000,                           // Channel layout tag values in this range are reserved for internal use
    kAudioChannelLayoutTag_EndReserved               = 0xFFFEFFFF,
};

enum
{
    kAudioFormatLinearPCM               = 'lpcm',
    kAudioFormatAC3                     = 'ac-3',
    kAudioFormat60958AC3                = 'cac3',
    kAudioFormatAppleIMA4               = 'ima4',
    kAudioFormatMPEG4AAC                = 'aac ',
    kAudioFormatMPEG4CELP               = 'celp',
    kAudioFormatMPEG4HVXC               = 'hvxc',
    kAudioFormatMPEG4TwinVQ             = 'twvq',
    kAudioFormatMACE3                   = 'MAC3',
    kAudioFormatMACE6                   = 'MAC6',
    kAudioFormatULaw                    = 'ulaw',
    kAudioFormatALaw                    = 'alaw',
    kAudioFormatQDesign                 = 'QDMC',
    kAudioFormatQDesign2                = 'QDM2',
    kAudioFormatQUALCOMM                = 'Qclp',
    kAudioFormatMPEGLayer1              = '.mp1',
    kAudioFormatMPEGLayer2              = '.mp2',
    kAudioFormatMPEGLayer3              = '.mp3',
    kAudioFormatTimeCode                = 'time',
    kAudioFormatMIDIStream              = 'midi',
    kAudioFormatParameterValueStream    = 'apvs',
    kAudioFormatAppleLossless           = 'alac',
    kAudioFormatMPEG4AAC_HE             = 'aach',
    kAudioFormatMPEG4AAC_LD             = 'aacl',
    kAudioFormatMPEG4AAC_HE_V2          = 'aacp',
    kAudioFormatMPEG4AAC_Spatial        = 'aacs',
    kAudioFormatAMR                     = 'samr'
};
#endif
