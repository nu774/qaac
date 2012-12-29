#ifndef ExtAudioFileX_H
#define ExtAudioFileX_H

#include "CoreAudio/ExtAudioFile.h"
#include "CoreAudio/AudioConverter.h"
#include "cautil.h"

class ExtAudioFileX {
    std::shared_ptr<OpaqueExtAudioFileID> m_file;
public:
    ExtAudioFileX() {}
    ExtAudioFileX(ExtAudioFileRef file, bool takeOwn)
    {
        attach(file, takeOwn);
    }
    void attach(ExtAudioFileRef file, bool takeOwn)
    {
        struct F {
            static OSStatus dispose(ExtAudioFileRef) { return 0; }
        };
        m_file.reset(file, takeOwn ? ExtAudioFileDispose : F::dispose);
    }
    operator ExtAudioFileRef() { return m_file.get(); }

    // property accessors
    SInt64 getFileLengthFrames()
    {
        SInt64 value;
        UInt32 size = sizeof value;
        CHECKCA(ExtAudioFileGetProperty(m_file.get(),
                                        kExtAudioFileProperty_FileLengthFrames,
                                        &size, &value));
        return value;
    }
    void getClientDataFormat(AudioStreamBasicDescription *result)
    {
        UInt32 size = sizeof(AudioStreamBasicDescription);
        CHECKCA(ExtAudioFileGetProperty(m_file.get(),
                                        kExtAudioFileProperty_ClientDataFormat,
                                        &size, result));
    }
    void setClientDataFormat(const AudioStreamBasicDescription &desc)
    {
        CHECKCA(ExtAudioFileSetProperty(m_file.get(),
                                        kExtAudioFileProperty_ClientDataFormat,
                                        sizeof(desc), &desc));
    }
    void getFileDataFormat(AudioStreamBasicDescription *result)
    {
        UInt32 size = sizeof(AudioStreamBasicDescription);
        CHECKCA(ExtAudioFileGetProperty(m_file.get(),
                                        kExtAudioFileProperty_FileDataFormat,
                                        &size, result));
    }
    void getClientChannelLayout(std::shared_ptr<AudioChannelLayout> *layout)
    {
        UInt32 size;
        Boolean writable;
        CHECKCA(ExtAudioFileGetPropertyInfo(m_file.get(),
                kExtAudioFileProperty_ClientChannelLayout, &size, &writable));
        std::shared_ptr<AudioChannelLayout> acl(
            reinterpret_cast<AudioChannelLayout*>(std::malloc(size)),
            std::free);
        CHECKCA(ExtAudioFileGetProperty(m_file.get(),
                kExtAudioFileProperty_ClientChannelLayout, &size, acl.get()));
        layout->swap(acl);
    }
    void getFileChannelLayout(std::shared_ptr<AudioChannelLayout> *layout)
    {
        UInt32 size;
        Boolean writable;
        CHECKCA(ExtAudioFileGetPropertyInfo(m_file.get(),
                kExtAudioFileProperty_FileChannelLayout, &size, &writable));
        std::shared_ptr<AudioChannelLayout> acl(
            reinterpret_cast<AudioChannelLayout*>(std::malloc(size)),
            std::free);
        CHECKCA(ExtAudioFileGetProperty(m_file.get(),
                kExtAudioFileProperty_FileChannelLayout, &size, acl.get()));
        layout->swap(acl);
    }
    AudioConverterRef getAudioConverter()
    {
        AudioConverterRef result;
        UInt32 size = sizeof(AudioConverterRef);
        CHECKCA(ExtAudioFileGetProperty(m_file.get(),
                                        kExtAudioFileProperty_AudioConverter,
                                        &size, &result));
        return result;
    }
};

#endif
