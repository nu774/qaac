#ifndef _FLACSRC_H
#define _FLACSRC_H

#include <deque>
#include <FLAC/all.h>
#include "iointer.h"
#include "channel.h"

struct HINSTANCE__;

class FLACModule {
    typedef std::tr1::shared_ptr<HINSTANCE__> module_t;
    module_t m_module;
    bool m_loaded;
public:
    FLACModule(): m_loaded(false) {}
    explicit FLACModule(const std::wstring &path);
    bool loaded() const { return m_loaded; }

    FLAC__StreamDecoder *(*stream_decoder_new)();
    FLAC__bool (*stream_decoder_finish)(FLAC__StreamDecoder *);
    void (*stream_decoder_delete)(FLAC__StreamDecoder *);
    FLAC__StreamDecoderInitStatus (*stream_decoder_init_stream)(
	    FLAC__StreamDecoder *,
	    FLAC__StreamDecoderReadCallback,
	    FLAC__StreamDecoderSeekCallback,
	    FLAC__StreamDecoderTellCallback,
	    FLAC__StreamDecoderLengthCallback,
	    FLAC__StreamDecoderEofCallback,
	    FLAC__StreamDecoderWriteCallback,
	    FLAC__StreamDecoderMetadataCallback,
	    FLAC__StreamDecoderErrorCallback,
	    void *
    );
    FLAC__StreamDecoderInitStatus (*stream_decoder_init_ogg_stream)(
	    FLAC__StreamDecoder *,
	    FLAC__StreamDecoderReadCallback,
	    FLAC__StreamDecoderSeekCallback,
	    FLAC__StreamDecoderTellCallback,
	    FLAC__StreamDecoderLengthCallback,
	    FLAC__StreamDecoderEofCallback,
	    FLAC__StreamDecoderWriteCallback,
	    FLAC__StreamDecoderMetadataCallback,
	    FLAC__StreamDecoderErrorCallback,
	    void *
    );
    FLAC__bool (*stream_decoder_set_metadata_respond)(
	    FLAC__StreamDecoder *, FLAC__MetadataType);
    FLAC__bool (*stream_decoder_process_until_end_of_metadata)(
	    FLAC__StreamDecoder *);
    FLAC__StreamDecoderState (*stream_decoder_get_state)(
	    const FLAC__StreamDecoder *);
    FLAC__bool (*stream_decoder_process_single)(FLAC__StreamDecoder *);
    FLAC__bool (*stream_decoder_seek_absolute)(
	    FLAC__StreamDecoder *, FLAC__uint64);
};

class FLACSource : public ISource, public ITagParser {
    typedef std::tr1::shared_ptr<FLAC__StreamDecoder> decoder_t;
    FLACModule m_module;
    InputStream m_stream;
    decoder_t m_decoder;
    SampleFormat m_format;
    std::vector<std::deque<int32_t> > m_buffer;
    uint64_t m_duration;
    uint64_t m_samples_read;
    std::map<uint32_t, std::wstring> m_tags;
    std::wstring m_cuesheet;
    std::vector<std::pair<std::wstring, int64_t> > m_chapters;
    bool m_giveup;
public:
    FLACSource(const FLACModule &module, InputStream &stream);
    void setRange(int64_t start=0, int64_t length=-1);
    uint64_t length() const { return m_duration; }
    const SampleFormat &getSampleFormat() const { return m_format; }
    const std::vector<uint32_t> *getChannelMap() const { return 0; }
    const std::map<uint32_t, std::wstring> &getTags() const { return m_tags; }
    size_t readSamples(void *buffer, size_t nsamples);
    const std::vector<std::pair<std::wstring, int64_t> >
	*getChapters() const
    {
	if (m_chapters.size())
	    return &m_chapters;
	else
	    return 0;
    }
private:
    template <class MemorySink>
    size_t readSamplesT(void *buffer, size_t nsamples);
    void close_decoder(FLAC__StreamDecoder *decoder)
    {
	m_module.stream_decoder_finish(decoder);
	m_module.stream_decoder_delete(decoder);
    }
    static FLAC__StreamDecoderReadStatus staticReadCallback(
	    const FLAC__StreamDecoder *decoder,
	    FLAC__byte *buffer,
	    unsigned *bytes,
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
	readCallback(FLAC__byte *buffer, unsigned *bytes);
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
