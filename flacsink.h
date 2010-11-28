#ifndef _FLACSINK_H
#define _FLACSINK_H

#include <FLAC/all.h>
#include "iointer.h"
#include "flacmodule.h"

class FLACSink: public ISink {
    typedef std::tr1::shared_ptr<FLAC__StreamEncoder> encoder_t;
    FILE *m_fp;
    FLACModule m_module;
    encoder_t m_encoder;
    FLAC__StreamMetadata *m_meta[2];
    SampleFormat m_format;
public:
    FLACSink(FILE *fp, uint64_t duration, const SampleFormat &format,
	    const FLACModule &module,
	    const std::map<uint32_t, std::wstring> &tags);
    void writeSamples(const void *data, size_t length, size_t nsamples);
private:
    void closeEncoder(FLAC__StreamEncoder *encoder)
    {
	m_module.stream_encoder_finish(encoder);
	m_module.stream_encoder_delete(encoder);
    }
    static FLAC__StreamEncoderWriteStatus staticWriteCallback(
	    const FLAC__StreamEncoder *encoder,
	    const FLAC__byte *buffer, size_t bytes,
	    unsigned samples, unsigned current_frame, void *client_data)
    {
	FLACSink *self = reinterpret_cast<FLACSink*>(client_data);
	return self->writeCallback(buffer, bytes, samples, current_frame);
    }
    FLAC__StreamEncoderWriteStatus writeCallback(
	    const FLAC__byte *buffer, size_t bytes,
	    unsigned samples, unsigned current_frame)
    {
	if (std::fwrite(buffer, 1, bytes, m_fp) == bytes)
	    return FLAC__STREAM_ENCODER_WRITE_STATUS_OK;
	else
	    return FLAC__STREAM_ENCODER_WRITE_STATUS_FATAL_ERROR;
    }
};

#endif
