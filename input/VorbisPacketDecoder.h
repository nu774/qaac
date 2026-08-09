#ifndef VORBISPACKETDECODER_H
#define VORBISPACKETDECODER_H

#include "PacketDecoder.h"
#include "dl.h"
#include <vorbis/codec.h>

class VorbisModule {
    DL m_dl;
private:
    VorbisModule() {
#ifdef _WIN32
        load("libvorbis.dll");
        if (!loaded()) load("libvorbis-0.dll");
        if (!loaded()) load("vorbis.dll");
#elif defined(__APPLE__)
        // see WavpackSource.h for why /opt/homebrew/lib needs a direct try
        if (!load("libvorbis.dylib"))
            load("/opt/homebrew/lib/libvorbis.dylib");
#else
        load("libvorbis.so");
        if (!loaded()) load("libvorbis.so.0");
#endif
    }
    VorbisModule(const VorbisModule&);
    VorbisModule& operator=(const VorbisModule&);
public:
    static VorbisModule &instance() {
        static VorbisModule self;
        return self;
    }
    bool load(const std::string &path);
    bool loaded() const { return m_dl.loaded(); }

    const char *(*version_string)();
    void (*info_init)(vorbis_info *);
    void (*info_clear)(vorbis_info *);
    void (*comment_init)(vorbis_comment *);
    void (*comment_clear)(vorbis_comment *);
    int (*synthesis_headerin)(vorbis_info *, vorbis_comment *, ogg_packet *);
    int (*synthesis_init)(vorbis_dsp_state *, vorbis_info *);
    int (*synthesis_restart)(vorbis_dsp_state *);
    int (*synthesis)(vorbis_block *, ogg_packet *);
    int (*synthesis_blockin)(vorbis_dsp_state *, vorbis_block *);
    int (*synthesis_pcmout)(vorbis_dsp_state *, float ***);
    int (*synthesis_read)(vorbis_dsp_state *, int);
    int (*block_init)(vorbis_dsp_state *, vorbis_block *);
    int (*block_clear)(vorbis_block *);
    void (*dsp_clear)(vorbis_dsp_state *);
};

class VorbisPacketDecoder: public IPacketDecoder {
    VorbisModule &m_module;
    vorbis_info m_info;
    vorbis_comment m_comment;
    vorbis_dsp_state m_dsp;
    vorbis_block m_block;
    bool m_headerDone;
    ca::AudioStreamBasicDescription m_iasbd, m_oasbd;
public:
    VorbisPacketDecoder();
    ~VorbisPacketDecoder();
    void reset();
    const ca::AudioStreamBasicDescription &getInputFormat() { return m_iasbd; }
    const ca::AudioStreamBasicDescription &getSampleFormat() { return m_oasbd; }
    void setMagicCookie(const std::vector<uint8_t> &cookie);
    size_t decode(const std::vector<uint8_t> &packet, std::vector<uint8_t> *samples);
};

#endif
