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
        if (takeOwn)
            m_file.reset(file, ExtAudioFileDispose);
        else
            m_file.reset(file, [](ExtAudioFileRef){});
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
    AudioStreamBasicDescription getClientDataFormat()
    {
        AudioStreamBasicDescription result;
        UInt32 size = sizeof(AudioStreamBasicDescription);
        CHECKCA(ExtAudioFileGetProperty(m_file.get(),
                                        kExtAudioFileProperty_ClientDataFormat,
                                        &size, &result));
        return result;
    }
    void setClientDataFormat(const AudioStreamBasicDescription &desc)
    {
        CHECKCA(ExtAudioFileSetProperty(m_file.get(),
                                        kExtAudioFileProperty_ClientDataFormat,
                                        sizeof(desc), &desc));
    }
    AudioStreamBasicDescription getFileDataFormat()
    {
        AudioStreamBasicDescription result;
        UInt32 size = sizeof(AudioStreamBasicDescription);
        CHECKCA(ExtAudioFileGetProperty(m_file.get(),
                                        kExtAudioFileProperty_FileDataFormat,
                                        &size, &result));
        return result;
    }
    std::shared_ptr<AudioChannelLayout> getClientChannelLayout()
    {
        UInt32 size;
        Boolean writable;
        CHECKCA(ExtAudioFileGetPropertyInfo(m_file.get(),
                kExtAudioFileProperty_ClientChannelLayout, &size, &writable));
        std::shared_ptr<AudioChannelLayout> acl(
            static_cast<AudioChannelLayout*>(std::malloc(size)),
            std::free);
        CHECKCA(ExtAudioFileGetProperty(m_file.get(),
                kExtAudioFileProperty_ClientChannelLayout, &size, acl.get()));
        return acl;
    }
    std::shared_ptr<AudioChannelLayout> getFileChannelLayout()
    {
        UInt32 size;
        Boolean writable;
        CHECKCA(ExtAudioFileGetPropertyInfo(m_file.get(),
                kExtAudioFileProperty_FileChannelLayout, &size, &writable));
        std::shared_ptr<AudioChannelLayout> acl(
            static_cast<AudioChannelLayout*>(std::malloc(size)),
            std::free);
        CHECKCA(ExtAudioFileGetProperty(m_file.get(),
                kExtAudioFileProperty_FileChannelLayout, &size, acl.get()));
        return acl;
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
