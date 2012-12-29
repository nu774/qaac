#ifndef AudioComponent_H
#define AudioComponent_H

#include "CoreFoundation.h"

#ifdef __cplusplus
extern "C" {
#endif

struct OpaqueAudioComponent;
typedef struct OpaqueAudioComponent *AudioComponent;

struct OpaqueAudioComponentInstance;
typedef struct OpaqueAudioComponentInstance *AudioComponentInstance;

typedef struct AudioComponentDescription {
   OSType  componentType;
   OSType  componentSubType;
   OSType  componentManufacturer;
   UInt32  componentFlags;
   UInt32  componentFlagsMask;
} AudioComponentDescription;

AudioComponent AudioComponentFindNext(
   AudioComponent             inAComponent,
   AudioComponentDescription  *inDesc
);

OSStatus AudioComponentGetDescription(
   AudioComponent             inComponent,
   AudioComponentDescription  *outDesc
);

OSStatus AudioComponentGetVersion(
   AudioComponent  inComponent,
   UInt32          *outVersion
);

OSStatus AudioComponentInstanceDispose(
   AudioComponentInstance inInstance
);

OSStatus AudioComponentInstanceNew(
   AudioComponent          inComponent,
   AudioComponentInstance  *outInstance
);

typedef void *CFPlugInFactoryFunction; // XXX

AudioComponent AudioComponentRegister(
   const AudioComponentDescription *inDesc,
   CFStringRef inName,
   UInt32 inVersion,
   CFPlugInFactoryFunction inFactory
);

#ifdef __cplusplus
}
#endif

#endif
