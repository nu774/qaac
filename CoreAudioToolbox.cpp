#include "win32util.h"
#include "CoreAudioToolbox.h"

#define CHECK(expr) do { if (!(expr)) throw std::runtime_error(#expr); } \
    while (0)

AudioComponent (*AudioComponentFindNext)(
   AudioComponent             inAComponent,
   AudioComponentDescription  *inDesc);

OSStatus (*AudioComponentGetDescription) (
   AudioComponent             inComponent,
   AudioComponentDescription  *outDesc);

OSStatus (*AudioComponentGetVersion) (
   AudioComponent  inComponent,
   UInt32          *outVersion);

OSStatus (*AudioComponentInstanceDispose) (
   AudioComponentInstance inInstance);

OSStatus (*AudioComponentInstanceNew) (
   AudioComponent          inComponent,
   AudioComponentInstance  *outInstance);

AudioComponent (*AudioComponentRegister)(
	const AudioComponentDescription *inDesc,
	CFStringRef inName,
	UInt32 inVersion,
	CFPlugInFactoryFunction inFactory);

OSStatus (*AudioConverterNew) (
   const AudioStreamBasicDescription    *inSourceFormat,
   const AudioStreamBasicDescription    *inDestinationFormat,
   AudioConverterRef                    *outAudioConverter);

OSStatus (*AudioConverterDispose) (
   AudioConverterRef inAudioConverter);

OSStatus (*AudioConverterGetProperty) (
   AudioConverterRef         inAudioConverter,
   AudioConverterPropertyID  inPropertyID,
   UInt32                    *ioPropertyDataSize,
   void                      *outPropertyData);

OSStatus (*AudioConverterGetPropertyInfo) (
   AudioConverterRef         inAudioConverter,
   AudioConverterPropertyID  inPropertyID,
   UInt32                    *outSize,
   Boolean                   *outWritable);

OSStatus (*AudioConverterSetProperty) (
   AudioConverterRef        inAudioConverter,
   AudioConverterPropertyID inPropertyID,
   UInt32                   inPropertyDataSize,
   const void               *inPropertyData);

OSStatus (*AudioConverterFillComplexBuffer)(
   AudioConverterRef                   inAudioConverter,
   AudioConverterComplexInputDataProc  inInputDataProc,
   void                                *inInputDataProcUserData,
   UInt32                              *ioOutputDataPacketSize,
   AudioBufferList                     *outOutputData,
   AudioStreamPacketDescription        *outPacketDescription);

void InitCoreAudioToolbox(const std::wstring &path)
{
    HMODULE hDll = LoadLibraryW(path.c_str());
    if (!hDll)
	throw std::runtime_error(format("Can't load %ls", path.c_str()));
    try {
	CHECK(AudioComponentFindNext =
		ProcAddress(hDll, "AudioComponentFindNext"));
	CHECK(AudioComponentGetDescription =
		ProcAddress(hDll, "AudioComponentGetDescription"));
	CHECK(AudioComponentGetVersion =
		ProcAddress(hDll, "AudioComponentGetVersion"));
	CHECK(AudioComponentInstanceDispose =
		ProcAddress(hDll, "AudioComponentInstanceDispose"));
	CHECK(AudioComponentInstanceNew =
		ProcAddress(hDll, "AudioComponentInstanceNew"));
	CHECK(AudioComponentRegister =
		ProcAddress(hDll, "AudioComponentRegister"));

	CHECK(AudioConverterNew = ProcAddress(hDll, "AudioConverterNew"));
	CHECK(AudioConverterDispose =
		ProcAddress(hDll, "AudioConverterDispose"));
	CHECK(AudioConverterGetProperty =
		ProcAddress(hDll, "AudioConverterGetProperty"));
	CHECK(AudioConverterSetProperty =
		ProcAddress(hDll, "AudioConverterSetProperty"));
	CHECK(AudioConverterFillComplexBuffer =
		ProcAddress(hDll, "AudioConverterFillComplexBuffer"));

	CFPlugInFactoryFunction aachFactory =
	    ProcAddress(hDll, "ACMP4AACHighEfficiencyEncoderFactory");
	if (aachFactory) {
	    AudioComponentDescription desc = { 'aenc', 'aach', 0 };
	    AudioComponent comp = AudioComponentRegister(&desc, CFSTR("AAC-HE Encoder"), 0, aachFactory);
	    int x = 1 + 1;
	}
    } catch (...) {
	throw;
    }
}
