#ifndef _FLACSRC_H
#define _FLACSRC_H

#include <deque>
#include <FLAC/all.h>
#include "iointer.h"
#include "flacmodule.h"

class FLACSource: public ISeekableSource, public ITagParser
{
    typedef std::shared_ptr<FLAC__StreamDecoder> decoder_t;
    bool m_eof;
    bool m_giveup;
    bool m_initialize_done;
    decoder_t m_decoder;
    uint64_t m_length;
    int64_t m_position;
    std::shared_ptr<FILE> m_fp;
    std::vector<uint32_t> m_chanmap;
    std::map<std::string, std::string> m_tags;
    std::vector<chapters::entry_t> m_chapters;
    util::FIFO<int32_t> m_buffer;
    AudioStreamBasicDescription m_asbd;
    FLACModule m_module;
public:
    FLACSource(const FLACModule &module, const std::shared_ptr<FILE> &fp);
    ~FLACSource() { m_decoder.reset(); }
    uint64_t length() const { return m_length; }
    const AudioStreamBasicDescription &getSampleFormat() const
    {
        return m_asbd;
    }
    const std::vector<uint32_t> *getChannels() const
    {
        return m_chanmap.size() ? &m_chanmap : 0;
    }
    int64_t getPosition() { return m_position; }
    size_t readSamples(void *buffer, size_t nsamples);
    bool isSeekable() { return util::is_seekable(fileno(m_fp.get())); }
    void seekTo(int64_t count);
    const std::map<std::string, std::string> &getTags() const { return m_tags; }
    const std::vector<chapters::entry_t> *getChapters() const
    {
        return m_chapters.size() ? &m_chapters : 0;
    }
private:
    void close(FLAC__StreamDecoder *decoder)
    {
        m_module.stream_decoder_finish(decoder);
        m_module.stream_decoder_delete(decoder);
    }
    static FLAC__StreamDecoderReadStatus staticReadCallback(
            const FLAC__StreamDecoder *decoder,
            FLAC__byte *buffer,
            size_t *bytes,
            void *client_data)
    {
        FLACSource *self = reinterpret_cast<FLACSource*>(client_data);
        return self->readCallback(buffer, bytes);
    }
    static FLAC__StreamDecoderSeekStatus staticSeekCallback(
            const FLAC__StreamDecoder *decoder,
            FLAC__uint64 offset,
            void *client_data)
    {
        FLACSource *self = reinterpret_cast<FLACSource*>(client_data);
        return self->seekCallback(offset);
    }
    static FLAC__StreamDecoderTellStatus staticTellCallback(
            const FLAC__StreamDecoder *decoder,
            FLAC__uint64 *offset,
            void *client_data)
    {
        FLACSource *self = reinterpret_cast<FLACSource*>(client_data);
        return self->tellCallback(offset);
    }
    static FLAC__StreamDecoderLengthStatus staticLengthCallback(
            const FLAC__StreamDecoder *decoder,
            FLAC__uint64 *length,
            void *client_data)
    {
        FLACSource *self = reinterpret_cast<FLACSource*>(client_data);
        return self->lengthCallback(length);
    }
    static FLAC__bool staticEofCallback(
            const FLAC__StreamDecoder *decoder, void *client_data)
    {
        FLACSource *self = reinterpret_cast<FLACSource*>(client_data);
        return self->eofCallback();
    }
    static FLAC__StreamDecoderWriteStatus staticWriteCallback(
            const FLAC__StreamDecoder *decoder,
            const FLAC__Frame *frame,
            const FLAC__int32 * const *buffer,
            void *client_data)
    {
        FLACSource *self = reinterpret_cast<FLACSource*>(client_data);
        return self->writeCallback(frame, buffer);
    }
    static void staticMetadataCallback(
            const FLAC__StreamDecoder *decoder,
            const FLAC__StreamMetadata *metadata,
            void *client_data)
    {
        FLACSource *self = reinterpret_cast<FLACSource*>(client_data);
        self->metadataCallback(metadata);
    }
    static void staticErrorCallback(
            const FLAC__StreamDecoder *decoder,
            FLAC__StreamDecoderErrorStatus status,
            void *client_data)
    {
        FLACSource *self = reinterpret_cast<FLACSource*>(client_data);
        self->errorCallback(status);
    }
    FLAC__StreamDecoderReadStatus
        readCallback(FLAC__byte *buffer, size_t *bytes);
    FLAC__StreamDecoderSeekStatus seekCallback(uint64_t offset);
    FLAC__StreamDecoderTellStatus tellCallback(uint64_t *offset);
    FLAC__StreamDecoderLengthStatus lengthCallback(uint64_t *length);
    FLAC__bool eofCallback();
    FLAC__StreamDecoderWriteStatus writeCallback(const FLAC__Frame *frame,
                const FLAC__int32 *const * buffer);
    void metadataCallback(const FLAC__StreamMetadata *metadata);
    void errorCallback(FLAC__StreamDecoderErrorStatus status);
    void handleStreamInfo(const FLAC__StreamMetadata_StreamInfo &si);
    void handleVorbisComment(const FLAC__StreamMetadata_VorbisComment &vc);
};

#endif
