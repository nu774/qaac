#ifndef AudioFile_H
#define AudioFile_H

#include "CoreFoundation.h"
#include "CoreAudioTypes.h"

#ifdef __cplusplus
extern "C" {
#endif

struct OpaqueAudioFileID;
typedef struct OpaqueAudioFileID *AudioFileID;

typedef UInt32 AudioFileTypeID;
typedef UInt32 AudioFilePropertyID;

struct AudioFile_SMPTE_Time {
   SInt8   mHours;
   UInt8   mMinutes;
   UInt8   mSeconds;
   UInt8   mFrames;
   UInt32  mSubFrameSampleOffset;
};
typedef struct AudioFile_SMPTE_Time AudioFile_SMPTE_Time;

struct AudioFileMarker {
   Float64 mFramePosition;
   CFStringRef           mName;
   SInt32                mMarkerID;
   AudioFile_SMPTE_Time  mSMPTETime;
   UInt32                mType;
   UInt16                mReserved;
   UInt16                mChannel;
};
typedef struct AudioFileMarker AudioFileMarker;

struct AudioFileMarkerList {
   UInt32           mSMPTE_TimeType;
   UInt32           mNumberMarkers;
   AudioFileMarker  mMarkers[1];
};
typedef struct AudioFileMarkerList AudioFileMarkerList;

struct AudioFileRegion {
   UInt32            mRegionID;
   CFStringRef       mName;
   UInt32            mFlags;
   UInt32            mNumberMarkers;
   AudioFileMarker   mMarkers[1];
};
typedef struct AudioFileRegion AudioFileRegion;

struct AudioFileRegionList {
   UInt32            mSMPTE_TimeType;
   UInt32            mNumberRegions;
   AudioFileRegion   mRegions[1];
};
typedef struct AudioFileRegionList AudioFileRegionList;

struct AudioFramePacketTranslation {
   SInt64  mFrame;
   SInt64  mPacket;
   UInt32  mFrameOffsetInPacket;
};
typedef struct AudioFramePacketTranslation AudioFramePacketTranslation;

struct AudioBytePacketTranslation {
   SInt64 mByte;
   SInt64 mPacket;
   UInt32 mByteOffsetInPacket;
   UInt32 mFlags;
};
typedef struct AudioBytePacketTranslation AudioBytePacketTranslation;

struct AudioFilePacketTableInfo {
   SInt64  mNumberValidFrames;
   SInt32  mPrimingFrames;
   SInt32  mRemainderFrames;
};
typedef struct AudioFilePacketTableInfo AudioFilePacketTableInfo;

struct AudioFileTypeAndFormatID {
   AudioFileTypeID  mFileType;
   UInt32           mFormatID;
};
typedef struct AudioFileTypeAndFormatID AudioFileTypeAndFormatID;

enum {
   kAudioFileAIFFType            = 'AIFF',
   kAudioFileAIFCType            = 'AIFC',
   kAudioFileWAVEType            = 'WAVE',
   kAudioFileSoundDesigner2Type  = 'Sd2f',
   kAudioFileNextType            = 'NeXT',
   kAudioFileMP3Type             = 'MPG3',
   kAudioFileMP2Type             = 'MPG2',
   kAudioFileMP1Type             = 'MPG1',
   kAudioFileAC3Type             = 'ac-3',
   kAudioFileAAC_ADTSType        = 'adts',
   kAudioFileMPEG4Type           = 'mp4f',
   kAudioFileM4AType             = 'm4af',
   kAudioFileCAFType             = 'caff',
   kAudioFile3GPType             = '3gpp',
   kAudioFile3GP2Type            = '3gp2',
   kAudioFileAMRType             = 'amrf'
};

enum {
   kAudioFilePropertyFileFormat            = 'ffmt',
   kAudioFilePropertyDataFormat            = 'dfmt',
   kAudioFilePropertyIsOptimized           = 'optm',
   kAudioFilePropertyMagicCookieData       = 'mgic',
   kAudioFilePropertyAudioDataByteCount    = 'bcnt',
   kAudioFilePropertyAudioDataPacketCount  = 'pcnt',
   kAudioFilePropertyMaximumPacketSize     = 'psze',
   kAudioFilePropertyDataOffset            = 'doff',
   kAudioFilePropertyChannelLayout         = 'cmap',
   kAudioFilePropertyDeferSizeUpdates      = 'dszu',
   kAudioFilePropertyDataFormatName        = 'fnme',
   kAudioFilePropertyMarkerList            = 'mkls',
   kAudioFilePropertyRegionList            = 'rgls',
   kAudioFilePropertyPacketToFrame         = 'pkfr',
   kAudioFilePropertyFrameToPacket         = 'frpk',
   kAudioFilePropertyPacketToByte          = 'pkby',
   kAudioFilePropertyByteToPacket          = 'bypk',
   kAudioFilePropertyChunkIDs              = 'chid',
   kAudioFilePropertyInfoDictionary        = 'info',
   kAudioFilePropertyPacketTableInfo       = 'pnfo',
   kAudioFilePropertyFormatList            = 'flst',
   kAudioFilePropertyPacketSizeUpperBound  = 'pkub',
   kAudioFilePropertyReserveDuration       = 'rsrv',
   kAudioFilePropertyEstimatedDuration     = 'edur',
   kAudioFilePropertyBitRate               = 'brat',
   kAudioFilePropertyID3Tag                = 'id3t',
   kAudioFilePropertySourceBitDepth        = 'sbtd',
   kAudioFilePropertyAlbumArtwork          = 'aart'
};

enum
{
   kAudioFileGlobalInfo_ReadableTypes                            = 'afrf',
   kAudioFileGlobalInfo_WritableTypes                            = 'afwf',
   kAudioFileGlobalInfo_FileTypeName                             = 'ftnm',
   kAudioFileGlobalInfo_AvailableStreamDescriptionsForFormat     = 'sdid',
   kAudioFileGlobalInfo_AvailableFormatIDs                       = 'fmid',
   
   kAudioFileGlobalInfo_AllExtensions                            = 'alxt',
   kAudioFileGlobalInfo_AllHFSTypeCodes                          = 'ahfs',
   kAudioFileGlobalInfo_AllUTIs                                  = 'auti',
   kAudioFileGlobalInfo_AllMIMETypes                             = 'amim',
   
   kAudioFileGlobalInfo_ExtensionsForType                        = 'fext',
   kAudioFileGlobalInfo_HFSTypeCodesForType                      = 'fhfs',
   kAudioFileGlobalInfo_UTIsForType                              = 'futi',
   kAudioFileGlobalInfo_MIMETypesForType                         = 'fmim',
   
   kAudioFileGlobalInfo_TypesForMIMEType                        = 'tmim',
   kAudioFileGlobalInfo_TypesForUTI                             = 'tuti',
   kAudioFileGlobalInfo_TypesForHFSTypeCode                     = 'thfs',
   kAudioFileGlobalInfo_TypesForExtension                       = 'text'
};

enum {
   kAudioFileFlags_EraseFile                = 1,
   kAudioFileFlags_DontPageAlignAudioData   = 2
};

enum {
   kAudioFileReadPermission      = 0x01,
   kAudioFileWritePermission     = 0x02,
   kAudioFileReadWritePermission = 0x03
};

enum {
   kAudioFileLoopDirection_NoLooping = 0,
   kAudioFileLoopDirection_Forward = 1,
   kAudioFileLoopDirection_ForwardAndBackward = 2,
   kAudioFileLoopDirection_Backward = 3
};

enum {
   kAudioFileMarkerType_Generic = 0,
};

enum {
   kAudioFileRegionFlag_LoopEnable = 1,
   kAudioFileRegionFlag_PlayForward = 2,
   kAudioFileRegionFlag_PlayBackward = 4
};

enum {
   kBytePacketTranslationFlag_IsEstimate = 1
};

#define kAFInfoDictionary_Artist "artist"
#define kAFInfoDictionary_Album "album"
#define kAFInfoDictionary_Tempo "tempo"
#define kAFInfoDictionary_KeySignature "key signature"
#define kAFInfoDictionary_TimeSignature "time signature"
#define kAFInfoDictionary_TrackNumber "track number"
#define kAFInfoDictionary_Year "year"
#define kAFInfoDictionary_Composer "composer"
#define kAFInfoDictionary_Lyricist "lyricist"
#define kAFInfoDictionary_Genre "genre"
#define kAFInfoDictionary_Title "title"
#define kAFInfoDictionary_RecordedDate "recorded date"
#define kAFInfoDictionary_Comments "comments"
#define kAFInfoDictionary_Copyright "copyright"
#define kAFInfoDictionary_SourceEncoder "source encoder"
#define kAFInfoDictionary_EncodingApplication "encoding application"
#define kAFInfoDictionary_NominalBitRate "nominal bit rate"
#define kAFInfoDictionary_ChannelLayout "channel layout"
#define kAFInfoDictionary_ApproximateDurationInSeconds "approximate duration in seconds"
#define kAFInfoDictionary_SourceBitDepth "source bit depth"
#define kAFInfoDictionary_ISRC "ISRC"
#define kAFInfoDictionary_SubTitle "subtitle"

typedef SInt64 (*AudioFile_GetSizeProc)(
   void  *inClientData
);

typedef OSStatus (*AudioFile_ReadProc) (
   void     *inClientData,
   SInt64   inPosition,
   UInt32   requestCount,
   void     *buffer,
   UInt32   *actualCount
);

typedef OSStatus (*AudioFile_SetSizeProc)(
   void     *inClientData,
   SInt64   inSize
);

typedef OSStatus (*AudioFile_WriteProc) (
   void          *inClientData,
   SInt64        inPosition,
   UInt32        requestCount,
   const void    *buffer,
   UInt32        *actualCount
);


OSStatus AudioFileClose (
   AudioFileID inAudioFile
);

OSStatus AudioFileCountUserData (
   AudioFileID inAudioFile,
   UInt32      inUserDataID,
   UInt32      *outNumberItems
);

/*
OSStatus AudioFileCreateWithURL (
   CFURLRef                          inFileRef,
   AudioFileTypeID                   inFileType,
   const AudioStreamBasicDescription *inFormat,
   UInt32                            inFlags,
   AudioFileID                       *outAudioFile
);
*/

OSStatus AudioFileGetGlobalInfo (
   AudioFilePropertyID inPropertyID,
   UInt32              inSpecifierSize,
   void                *inSpecifier,
   UInt32              *ioDataSize,
   void                *outPropertyData
);

OSStatus AudioFileGetGlobalInfoSize (
   AudioFilePropertyID inPropertyID,
   UInt32              inSpecifierSize,
   void                *inSpecifier,
   UInt32              *outDataSize
);

OSStatus AudioFileGetProperty (
   AudioFileID         inAudioFile,
   AudioFilePropertyID inPropertyID,
   UInt32              *ioDataSize,
   void                *outPropertyData
);

OSStatus AudioFileGetPropertyInfo (
   AudioFileID         inAudioFile,
   AudioFilePropertyID inPropertyID,
   UInt32              *outDataSize,
   UInt32              *isWritable
);

OSStatus AudioFileGetUserData (
   AudioFileID inAudioFile,
   UInt32      inUserDataID,
   UInt32      inIndex,
   UInt32      *ioUserDataSize,
   void        *outUserData
);

OSStatus AudioFileGetUserDataSize (
   AudioFileID inAudioFile,
   UInt32      inUserDataID,
   UInt32      inIndex,
   UInt32      *outUserDataSize
);

OSStatus AudioFileInitializeWithCallbacks (
   void                              *inClientData,
   AudioFile_ReadProc                inReadFunc,
   AudioFile_WriteProc               inWriteFunc,
   AudioFile_GetSizeProc             inGetSizeFunc,
   AudioFile_SetSizeProc             inSetSizeFunc,
   AudioFileTypeID                   inFileType,
   const AudioStreamBasicDescription *inFormat,
   UInt32                            inFlags,
   AudioFileID                       *outAudioFile
);

OSStatus AudioFileOpenURL (
   CFURLRef        inFileRef,
   SInt8           inPermissions,
   AudioFileTypeID inFileTypeHint,
   AudioFileID     *outAudioFile
);

OSStatus AudioFileOpenWithCallbacks (
   void                  *inClientData,
   AudioFile_ReadProc    inReadFunc,
   AudioFile_WriteProc   inWriteFunc,
   AudioFile_GetSizeProc inGetSizeFunc,
   AudioFile_SetSizeProc inSetSizeFunc,
   AudioFileTypeID       inFileTypeHint,
   AudioFileID           *outAudioFile
);

OSStatus AudioFileOptimize (
   AudioFileID inAudioFile
);

OSStatus AudioFileReadBytes (
   AudioFileID inAudioFile,
   Boolean   inUseCache,
   SInt64    inStartingByte,
   UInt32    *ioNumBytes,
   void      *outBuffer
);

OSStatus AudioFileReadPacketData (
   AudioFileID                   inAudioFile,
   Boolean                       inUseCache,
   UInt32                        *ioNumBytes,
   AudioStreamPacketDescription  *outPacketDescriptions,
   SInt64                        inStartingPacket,
   UInt32                        *ioNumPackets,
   void                          *outBuffer
);

OSStatus AudioFileReadPackets (
   AudioFileID                  inAudioFile,
   Boolean                      inUseCache,
   UInt32                       *outNumBytes,
   AudioStreamPacketDescription *outPacketDescriptions,
   SInt64                       inStartingPacket,
   UInt32                       *ioNumPackets,
   void                         *outBuffer
);

OSStatus AudioFileRemoveUserData (
   AudioFileID inAudioFile,
   UInt32      inUserDataID,
   UInt32      inIndex
);

OSStatus AudioFileSetProperty (
    AudioFileID         inAudioFile,
    AudioFilePropertyID inPropertyID,
    UInt32              inDataSize,
    const void          *inPropertyData
);

OSStatus AudioFileSetUserData (
   AudioFileID inAudioFile,
   UInt32      inUserDataID,
   UInt32      inIndex,
   UInt32      inUserDataSize,
   const void  *inUserData
);

OSStatus AudioFileWriteBytes (
   AudioFileID inAudioFile,
   Boolean     inUseCache,
   SInt64      inStartingByte,
   UInt32      *ioNumBytes,
   const void  *inBuffer
);

OSStatus AudioFileWritePackets (
   AudioFileID                        inAudioFile,
   Boolean                            inUseCache,
   UInt32                             inNumBytes,
   const AudioStreamPacketDescription *inPacketDescriptions,
   SInt64                             inStartingPacket,
   UInt32                             *ioNumPackets,
   const void                         *inBuffer
);

#ifdef __cplusplus
}
#endif

#endif
