#ifndef ExtAudioFile_H
#define ExtAudioFile_H

#include "CoreAudioTypes.h"
#include "AudioFile.h"

#ifdef __cplusplus
extern "C" {
#endif

struct OpaqueExtAudioFileID;
typedef struct OpaqueExtAudioFileID *ExtAudioFileRef;

typedef UInt32 ExtAudioFilePropertyID;

enum {
   kExtAudioFileProperty_FileDataFormat       = 'ffmt',
   kExtAudioFileProperty_FileChannelLayout    = 'fclo',
   kExtAudioFileProperty_ClientDataFormat     = 'cfmt',
   kExtAudioFileProperty_ClientChannelLayout  = 'cclo',
   kExtAudioFileProperty_CodecManufacturer    = 'cman',
   
   // read-only properties:
   kExtAudioFileProperty_AudioConverter       = 'acnv',
   kExtAudioFileProperty_AudioFile            = 'afil',
   kExtAudioFileProperty_FileMaxPacketSize    = 'fmps',
   kExtAudioFileProperty_ClientMaxPacketSize  = 'cmps',
   kExtAudioFileProperty_FileLengthFrames     = '#frm',
   
   // read/write properties:
   kExtAudioFileProperty_ConverterConfig      = 'accf',
   kExtAudioFileProperty_IOBufferSizeBytes    = 'iobs',
   kExtAudioFileProperty_IOBuffer             = 'iobf',
   kExtAudioFileProperty_PacketTable          = 'xpti'
};

OSStatus ExtAudioFileCreateWithURL (
   CFURLRef                          inURL,
   AudioFileTypeID                   inFileType,
   const AudioStreamBasicDescription *inStreamDesc,
   const AudioChannelLayout          *inChannelLayout,
   UInt32                            inFlags,
   ExtAudioFileRef                   *outExtAudioFile
);

OSStatus ExtAudioFileDispose (
   ExtAudioFileRef inExtAudioFile
);

OSStatus ExtAudioFileGetProperty (
   ExtAudioFileRef         inExtAudioFile,
   ExtAudioFilePropertyID  inPropertyID,
   UInt32                  *ioPropertyDataSize,
   void                    *outPropertyData
);

OSStatus ExtAudioFileGetPropertyInfo (
   ExtAudioFileRef         inExtAudioFile,
   ExtAudioFilePropertyID  inPropertyID,
   UInt32                  *outSize,
   Boolean                 *outWritable
);

OSStatus ExtAudioFileOpenURL (
   CFURLRef         inURL,
   ExtAudioFileRef  *outExtAudioFile
);

OSStatus ExtAudioFileRead (
   ExtAudioFileRef  inExtAudioFile,
   UInt32           *ioNumberFrames,
   AudioBufferList  *ioData
);

OSStatus ExtAudioFileSeek (
   ExtAudioFileRef  inExtAudioFile,
   SInt64           inFrameOffset
);

OSStatus ExtAudioFileSetProperty (
   ExtAudioFileRef         inExtAudioFile,
   ExtAudioFilePropertyID  inPropertyID,
   UInt32                  inPropertyDataSize,
   const void              *inPropertyData
);

OSStatus ExtAudioFileTell (
   ExtAudioFileRef  inExtAudioFile,
   SInt64           *outFrameOffset
);

OSStatus ExtAudioFileWrapAudioFileID (
   AudioFileID      inFileID,
   Boolean          inForWriting,
   ExtAudioFileRef  *outExtAudioFile
);

OSStatus ExtAudioFileWrite (
   ExtAudioFileRef        inExtAudioFile,
   UInt32                 inNumberFrames,
   const AudioBufferList  *ioData
);

OSStatus ExtAudioFileWriteAsync (
   ExtAudioFileRef        inExtAudioFile,
   UInt32                 inNumberFrames,
   const AudioBufferList  *ioData
);
#ifdef __cplusplus
}
#endif

#endif
