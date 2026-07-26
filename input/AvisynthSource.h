#ifndef AVSSRC_H
#define AVSSRC_H

#include "ISource.h"
#include "dl.h"
#include "avisynth_c.h"

class AvisynthModule {
    DL m_dl;
private:
    AvisynthModule() {
        load("avisynth.dll");
    }
    AvisynthModule(const AvisynthModule&);
    AvisynthModule& operator=(const AvisynthModule&);
public:
    static AvisynthModule &instance() {
        static AvisynthModule self;
        return self;
    }
    bool load(const std::string &path);
    bool loaded() const { return m_dl.loaded(); }

    AVS_ScriptEnvironment * (__stdcall *create_script_environment)(int);
    void (__stdcall *delete_script_environment)(AVS_ScriptEnvironment *);
    int (__stdcall *get_audio)(AVS_Clip *, void *, INT64, INT64);
    const char * (__stdcall *get_error)(AVS_ScriptEnvironment *);
    const AVS_VideoInfo * (__stdcall *get_video_info)(AVS_Clip *);
    AVS_Value (__stdcall *invoke)(AVS_ScriptEnvironment *, const char *,
                                  AVS_Value, const char **);
    void (__stdcall *release_clip)(AVS_Clip *);
    void (__stdcall *release_value)(AVS_Value);
    AVS_Clip * (__stdcall *take_clip)(AVS_Value, AVS_ScriptEnvironment *);
};

class AvisynthSource: public ISeekableSource
{
    uint64_t m_duration;
    uint64_t m_position;
    std::vector<uint8_t> m_buffer;
    std::shared_ptr<AVS_ScriptEnvironment> m_script_env;
    std::shared_ptr<AVS_Clip> m_clip;
    AudioStreamBasicDescription m_asbd;
    AvisynthModule &m_module;
public:
    AvisynthSource(const std::string &path);
    uint64_t length() const { return m_duration; }
    const AudioStreamBasicDescription &getSampleFormat() const
    {
        return m_asbd;
    }
    const std::vector<uint32_t> *getChannels() const { return 0; }
    int64_t getPosition() { return m_position; }
    size_t readSamples(void *buffer, size_t nsamples);
    bool isSeekable() { return true; }
    void seekTo(int64_t count) { m_position = count; }
};

#endif
