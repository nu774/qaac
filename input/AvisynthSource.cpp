#include "AvisynthSource.h"
#include "cautil.h"

#define CHECK(expr) do { if (!(expr)) throw std::runtime_error("!?"); } \
    while (0)
bool AvisynthModule::load(const std::wstring &path)
{
    if (!m_dl.load(path)) return false;
    try {
        CHECK(create_script_environment =
              m_dl.fetch("avs_create_script_environment"));
        CHECK(delete_script_environment = 
              m_dl.fetch("avs_delete_script_environment"));
        CHECK(get_audio = m_dl.fetch("avs_get_audio"));
        CHECK(get_error = m_dl.fetch("avs_get_error"));
        CHECK(get_video_info = m_dl.fetch("avs_get_video_info"));
        CHECK(invoke = m_dl.fetch("avs_invoke"));
        CHECK(release_clip = m_dl.fetch("avs_release_clip"));
        CHECK(release_value = m_dl.fetch("avs_release_value"));
        CHECK(take_clip = m_dl.fetch("avs_take_clip"));
        return true;
    } catch (...) {
        m_dl.reset();
        return false;
    }
}


AvisynthSource::AvisynthSource(const std::wstring &path)
    : m_position(0),
      m_module(AvisynthModule::instance())
{
    if (!m_module.loaded()) throw std::runtime_error("Avisynth not loaded");
    AVS_ScriptEnvironment *env = m_module.create_script_environment(2);
    m_script_env = std::shared_ptr<AVS_ScriptEnvironment>(env,
                                          m_module.delete_script_environment);
    const char *error = m_module.get_error(env);
    if (error)
        throw std::runtime_error(error);
    std::string spath;
    try {
        spath = strutil::w2m(path);
    } catch (...) {
        throw std::runtime_error("Unicode path is not available for avs");
    }
    AVS_Value arg = avs_new_value_string(spath.c_str());
    AVS_Value clip_value = m_module.invoke(env, "Import", arg, nullptr);
    if (avs_is_error(clip_value))
        throw std::runtime_error(avs_as_string(clip_value));
    AVS_Value mt_mode_value =
        m_module.invoke(env, "GetMTMode", avs_new_value_bool(0), nullptr);
    int mt_mode = avs_is_int(mt_mode_value) ? avs_as_int(mt_mode_value) : 0;
    m_module.release_value(mt_mode_value);
    if (mt_mode > 0 && mt_mode < 5) {
        AVS_Value value = m_module.invoke(env, "Distributor", clip_value, nullptr);
        m_module.release_value(clip_value);
        clip_value = value;
    }
    if (!avs_is_clip(clip_value)) {
        m_module.release_value(clip_value);
        throw std::runtime_error("Cannot get clip");
    }
    m_clip = std::shared_ptr<AVS_Clip>(m_module.take_clip(clip_value, env),
                                       m_module.release_clip);
    m_module.release_value(clip_value);
    const AVS_VideoInfo *vi = m_module.get_video_info(m_clip.get());
    if (!avs_has_audio(vi))
        throw std::runtime_error("No audio in the clip");
    bool is_float = avs_sample_type(vi) == AVS_SAMPLE_FLOAT;
    int bits = avs_bytes_per_channel_sample(vi) * 8;
    m_asbd = cautil::buildASBDForPCM2(avs_samples_per_second(vi),
                                      avs_audio_channels(vi),
                                      bits, 32,
                                      is_float ? kAudioFormatFlagIsFloat
                                           : kAudioFormatFlagIsSignedInteger);
    m_duration = vi->num_audio_samples;
}

size_t AvisynthSource::readSamples(void *buffer, size_t nsamples)
{
    int64_t rest = m_duration - m_position;
    nsamples = std::min(static_cast<int64_t>(nsamples), rest);
    const AVS_VideoInfo *vi = m_module.get_video_info(m_clip.get());
    ssize_t nbytes = nsamples * avs_bytes_per_audio_sample(vi);
    if (m_buffer.size() < nbytes)
        m_buffer.resize(nbytes);
    m_module.get_audio(m_clip.get(), m_buffer.data(), m_position, nsamples);
    if (nsamples) {
        size_t size = nsamples * avs_bytes_per_audio_sample(vi);
        util::unpack(m_buffer.data(), buffer, &size,
                     avs_bytes_per_channel_sample(vi),
                     m_asbd.mBytesPerFrame / m_asbd.mChannelsPerFrame);
        if (m_asbd.mBitsPerChannel <= 8) {
            util::convert_sign(static_cast<uint32_t *>(buffer),
                               nsamples * m_asbd.mChannelsPerFrame);
        }
        m_position += nsamples;
    }
    return nsamples;
}

