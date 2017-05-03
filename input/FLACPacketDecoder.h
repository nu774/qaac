#ifndef FLACPACKETDECODER_H
#define FLACPACKETDECODER_H

#include <memory>
#include "PacketDecoder.h"
#include "util.h"
#include "flacmodule.h"

class FLACPacketDecoder: public IPacketDecoder {
    typedef FLACPacketDecoder ThisType;
    typedef std::shared_ptr<FLAC__StreamDecoder> decoder_t;
    decoder_t m_decoder;
    IPacketFeeder *m_feeder;
    AudioStreamBasicDescription m_iasbd, m_oasbd;
    std::vector<uint8_t> m_feed_buffer;
    util::FIFO<uint8_t> m_packet_buffer;
    util::FIFO<int32_t> m_decode_buffer;
    FLACModule m_module;
public:
    FLACPacketDecoder(IPacketFeeder *feeder, const FLACModule &module);
    ~FLACPacketDecoder()
    {
        m_decoder.reset();
    }
    void reset();
    const AudioStreamBasicDescription &getInputFormat() { return m_iasbd; }
    const AudioStreamBasicDescription &getSampleFormat() { return m_oasbd; }
    void setMagicCookie(const std::vector<uint8_t> &cookie);
    size_t decode(void *data, size_t nsamples);
private:
    void close(FLAC__StreamDecoder *decoder)
    {
        m_module.stream_decoder_finish(decoder);
        m_module.stream_decoder_delete(decoder);
    }
    static FLAC__StreamDecoderReadStatus
        staticReadCallback(const FLAC__StreamDecoder *decoder,
                           FLAC__byte *buffer,
                           size_t *bytes,
                           void *client_data)
    {
        ThisType *self = reinterpret_cast<ThisType*>(client_data);
        return self->readCallback(buffer, bytes);
    }
    static FLAC__StreamDecoderSeekStatus
        staticSeekCallback(const FLAC__StreamDecoder *decoder,
                           FLAC__uint64 offset,
                           void *client_data)
    {
        return FLAC__STREAM_DECODER_SEEK_STATUS_UNSUPPORTED;
    }
    static FLAC__StreamDecoderTellStatus
        staticTellCallback(const FLAC__StreamDecoder *decoder,
                           FLAC__uint64 *offset,
                           void *client_data)
    {
        return FLAC__STREAM_DECODER_TELL_STATUS_UNSUPPORTED;
    }
    static FLAC__StreamDecoderLengthStatus
        staticLengthCallback(const FLAC__StreamDecoder *decoder,
                             FLAC__uint64 *length,
                             void *client_data)
    {
        return FLAC__STREAM_DECODER_LENGTH_STATUS_UNSUPPORTED;
    }
    static FLAC__bool staticEofCallback(const FLAC__StreamDecoder *decoder,
                                        void *client_data)
    {
        return 0;
    }
    static FLAC__StreamDecoderWriteStatus
        staticWriteCallback(const FLAC__StreamDecoder *decoder,
                            const FLAC__Frame *frame,
                            const FLAC__int32 * const *buffer,
                            void *client_data)
    {
        ThisType *self = reinterpret_cast<ThisType*>(client_data);
        return self->writeCallback(frame, buffer);
    }
    static void staticMetadataCallback(const FLAC__StreamDecoder *decoder,
                                       const FLAC__StreamMetadata *metadata,
                                       void *client_data)
    {
        ThisType *self = reinterpret_cast<ThisType*>(client_data);
        self->metadataCallback(metadata);
    }
    static void staticErrorCallback(const FLAC__StreamDecoder *decoder,
                                    FLAC__StreamDecoderErrorStatus status,
                                    void *client_data)
    {
        ThisType *self = reinterpret_cast<ThisType*>(client_data);
        self->errorCallback(status);
    }
    FLAC__StreamDecoderReadStatus
        readCallback(FLAC__byte *buffer, size_t *bytes);
    FLAC__bool eofCallback();
    FLAC__StreamDecoderWriteStatus
        writeCallback(const FLAC__Frame *frame,
                      const FLAC__int32 *const * buffer);
    void metadataCallback(const FLAC__StreamMetadata *metadata);
    void errorCallback(FLAC__StreamDecoderErrorStatus status);
};
#endif
