#ifndef _FLACSRC_H
#define _FLACSRC_H

#include <deque>
#include <FLAC/all.h>
#include "iointer.h"
#include "channel.h"
#include "flacmodule.h"

class FLACSource :
    public ISource, public ITagParser, public PartialSource<FLACSource>
{
    typedef std::tr1::shared_ptr<FLAC__StreamDecoder> decoder_t;
    FLACModule m_module;
    InputStream m_stream;
    decoder_t m_decoder;
    SampleFormat m_format;
    std::vector<std::deque<int32_t> > m_buffer;
    std::map<uint32_t, std::wstring> m_tags;
    std::wstring m_cuesheet;
    std::vector<std::pair<std::wstring, int64_t> > m_chapters;
    bool m_giveup;
public:
    FLACSource(const FLACModule &module, InputStream &stream);
    uint64_t length() const { return getDuration(); }
    const SampleFormat &getSampleFormat() const { return m_format; }
    const std::vector<uint32_t> *getChannelMap() const { return 0; }
    const std::map<uint32_t, std::wstring> &getTags() const { return m_tags; }
    size_t readSamples(void *buffer, size_t nsamples);
    void skipSamples(int64_t count);
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
