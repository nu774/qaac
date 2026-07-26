#ifndef _FLACMODULE_H
#define _FLACMODULE_H

#include <FLAC/all.h>
#include "dl.h"

class FLACModule {
    DL m_dl;
private:
    FLACModule() {
        load("libFLAC_dynamic.dll");
        if (!loaded()) load("libFLAC.dll");
        if (!loaded()) load("libFLAC-8.dll");
    }
    FLACModule(const FLACModule&);
    FLACModule& operator=(const FLACModule&);
public:
    static FLACModule &instance() {
        static FLACModule self;
        return self;
    }
    bool load(const std::string &path);
    bool loaded() const { return m_dl.loaded(); }

    const char *VERSION_STRING;
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
            void *);
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
            void *);
    FLAC__bool (*stream_decoder_set_metadata_respond)(FLAC__StreamDecoder *,
                                                      FLAC__MetadataType);
    FLAC__bool
    (*stream_decoder_process_until_end_of_metadata)(FLAC__StreamDecoder *);
    FLAC__StreamDecoderState
    (*stream_decoder_get_state)(const FLAC__StreamDecoder *);
    FLAC__bool (*stream_decoder_process_single)(FLAC__StreamDecoder *);
    FLAC__bool (*stream_decoder_seek_absolute)(FLAC__StreamDecoder *,
                                               FLAC__uint64);
    FLAC__bool (*stream_decoder_get_decode_position)(FLAC__StreamDecoder *,
                                                     FLAC__uint64*);
    FLAC__bool (*stream_decoder_reset)(FLAC__StreamDecoder *);
};

#endif
