#ifndef AudioFormat_H
#define AudioFormat_H

#include "CoreAudioTypes.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {
   // AudioStreamBasicDescription structure properties
   kAudioFormatProperty_FormatInfo                       = 'fmti',
   kAudioFormatProperty_FormatName                       = 'fnam',
   kAudioFormatProperty_EncodeFormatIDs                  = 'acof',
   kAudioFormatProperty_DecodeFormatIDs                  = 'acif',
   kAudioFormatProperty_FormatList                       = 'flst',
   kAudioFormatProperty_ASBDFromESDS                     = 'essd',
   kAudioFormatProperty_ChannelLayoutFromESDS            = 'escl',
   kAudioFormatProperty_OutputFormatList                 = 'ofls',
   kAudioFormatProperty_Encoders                         = 'aven',
   kAudioFormatProperty_Decoders                         = 'avde',
   kAudioFormatProperty_FormatIsVBR                      = 'fvbr',
   kAudioFormatProperty_FormatIsExternallyFramed         = 'fexf',
   kAudioFormatProperty_AvailableEncodeBitRates          = 'aebr',
   kAudioFormatProperty_AvailableEncodeSampleRates       = 'aesr',
   kAudioFormatProperty_AvailableEncodeChannelLayoutTags = 'aecl',
   kAudioFormatProperty_AvailableEncodeNumberChannels    = 'avnc',
   kAudioFormatProperty_ASBDFromMPEGPacket               = 'admp',
   //
   // AudioChannelLayout structure properties
   kAudioFormatProperty_BitmapForLayoutTag               = 'bmtg',
   kAudioFormatProperty_MatrixMixMap                     = 'mmap',
   kAudioFormatProperty_ChannelMap                       = 'chmp',
   kAudioFormatProperty_NumberOfChannelsForLayout        = 'nchm',
   kAudioFormatProperty_ValidateChannelLayout            = 'vacl',
   kAudioFormatProperty_ChannelLayoutForTag              = 'cmpl',
   kAudioFormatProperty_TagForChannelLayout              = 'cmpt',
   kAudioFormatProperty_ChannelLayoutName                = 'lonm',
   kAudioFormatProperty_ChannelLayoutSimpleName          = 'lsnm',
   kAudioFormatProperty_ChannelLayoutForBitmap           = 'cmpb',
   kAudioFormatProperty_ChannelName                      = 'cnam',
   kAudioFormatProperty_ChannelShortName                 = 'csnm',
   kAudioFormatProperty_TagsForNumberOfChannels          = 'tagc',
   kAudioFormatProperty_PanningMatrix                    = 'panm',
   kAudioFormatProperty_BalanceFade                      = 'balf',
   //
   // ID3 tag (MP3 metadata) properties
   kAudioFormatProperty_ID3TagSize                       = 'id3s',
   kAudioFormatProperty_ID3TagToDictionary               = 'id3d'
};

typedef UInt32 AudioFormatPropertyID;

struct AudioFormatListItem
{
    AudioStreamBasicDescription         mASBD;
    AudioChannelLayoutTag               mChannelLayoutTag;
};
typedef struct AudioFormatListItem AudioFormatListItem;

OSStatus AudioFormatGetProperty (
   AudioFormatPropertyID inPropertyID,
   UInt32                inSpecifierSize,
   const void            *inSpecifier,
   UInt32                *ioPropertyDataSize,
   void                  *outPropertyData
);

OSStatus AudioFormatGetPropertyInfo (
   AudioFormatPropertyID  inPropertyID,
   UInt32                 inSpecifierSize,
   const void             *inSpecifier,
   UInt32                 *outPropertyDataSize
);

#ifdef __cplusplus
}
#endif

#endif
