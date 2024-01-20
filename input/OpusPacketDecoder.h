#ifndef OPUSPACKETDECODER_H
#define OPUSPACKETDECODER_H

#include <memory>
#include "PacketDecoder.h"
#include "dl.h"
#include <opus_multistream.h>

struct HINSTANCE__;

class LibOpusModule {
    DL m_dl;
private:    
    LibOpusModule() {
        load(L"opus.dll");
        if (!loaded()) load(L"libopus-0.dll");
    }
    LibOpusModule(const LibOpusModule&);
    LibOpusModule& operator=(const LibOpusModule&);
public:
    static LibOpusModule &instance() {
        static LibOpusModule self;
        return self;
    }
    bool load(const std::wstring &path);
    bool loaded() const { return m_dl.loaded(); }
    
    const char *(*get_version_string)();
    const char *(*strerror)(int);
    int (*packet_get_nb_samples)(const unsigned char *, opus_int32, opus_int32);
    OpusMSDecoder *(*multistream_decoder_create)(opus_int32, int, int, int, const unsigned char *, int *);
    int (*multistream_decode_float)(OpusMSDecoder *, const unsigned char *, opus_int32, float *, int, int);
    int (*multistream_decoder_ctl)(OpusMSDecoder *, int, ...);
    void (*multistream_decoder_destroy)(OpusMSDecoder *);
};

class OpusPacketDecoder: public IPacketDecoder {
    LibOpusModule &m_module;
    IPacketFeeder *m_feeder;
    std::shared_ptr<OpusMSDecoder> m_decoder;
    AudioStreamBasicDescription m_iasbd, m_oasbd;
    std::vector<uint8_t> m_packet_buffer;
    util::FIFO<float> m_decode_buffer;
public:
    OpusPacketDecoder(IPacketFeeder *feeder);
    ~OpusPacketDecoder();
    void reset();
    const AudioStreamBasicDescription &getInputFormat() { return m_iasbd; }
    const AudioStreamBasicDescription &getSampleFormat() { return m_oasbd; }
    void setMagicCookie(const std::vector<uint8_t> &cookie);
    size_t decode(void *data, size_t nsamples);
};

#endif
