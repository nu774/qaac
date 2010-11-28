#ifndef _FLACMODULE_H
#define _FLACMODULE_H

#include <string>
#if _MSC_VER >= 1500
# include <memory>
#else
# include <boost/tr1/memory.hpp>
#endif
#include <FLAC/all.h>

struct HINSTANCE__;

class FLACModule {
    typedef std::tr1::shared_ptr<HINSTANCE__> module_t;
    module_t m_module;
    bool m_loaded;
public:
    FLACModule(): m_loaded(false) {}
    explicit FLACModule(const std::wstring &path);
    bool loaded() const { return m_loaded; }

    /* decoder interfaces */
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

    /* encoder interfaces */
    FLAC__StreamEncoder *(*stream_encoder_new)();
    void (*stream_encoder_delete)(FLAC__StreamEncoder *);
    FLAC__bool (*stream_encoder_set_channels)(FLAC__StreamEncoder *, unsigned);
    FLAC__bool (*stream_encoder_set_bits_per_sample)(
	    FLAC__StreamEncoder *, unsigned);
    FLAC__bool (*stream_encoder_set_sample_rate)(
	    FLAC__StreamEncoder *, unsigned);
    FLAC__bool (*stream_encoder_set_total_samples_estimate)(
	    FLAC__StreamEncoder *, FLAC__uint64);
    FLAC__bool (*stream_encoder_set_metadata)(
	    FLAC__StreamEncoder *, FLAC__StreamMetadata **, unsigned);
    FLAC__StreamEncoderState (*stream_encoder_get_state)(
	    const FLAC__StreamEncoder *);
    FLAC__bool (*stream_encoder_set_compression_level)(
	    FLAC__StreamEncoder *encoder, unsigned);
    FLAC__StreamEncoderInitStatus (*stream_encoder_init_stream)(
	    FLAC__StreamEncoder *,
	    FLAC__StreamEncoderWriteCallback,
	    FLAC__StreamEncoderSeekCallback,
	    FLAC__StreamEncoderTellCallback,
	    FLAC__StreamEncoderMetadataCallback,
	    void *
    );
    FLAC__bool (*stream_encoder_finish)(FLAC__StreamEncoder *);
    FLAC__bool (*stream_encoder_process_interleaved)(
	    FLAC__StreamEncoder *, const FLAC__int32 *, unsigned);

    /* metadata interfaces */
    FLAC__StreamMetadata *(*metadata_object_new)(FLAC__MetadataType);
    void (*metadata_object_delete)(FLAC__StreamMetadata *);
    FLAC__bool (*metadata_object_vorbiscomment_append_comment)(
	    FLAC__StreamMetadata *,
	    FLAC__StreamMetadata_VorbisComment_Entry,
	    FLAC__bool
    );
    FLAC__bool (*metadata_object_vorbiscomment_entry_from_name_value_pair)(
	    FLAC__StreamMetadata_VorbisComment_Entry *,
	    const char *, const char *
    );
};

#endif
