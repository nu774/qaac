#include "FLACModule.h"

#define CHECK(expr) do { if (!(expr)) throw std::runtime_error("!!!"); } \
    while (0)

bool FLACModule::load(const std::string &path)
{
    if (!m_dl.load(path)) return false;
    try {
        void *vp;
        CHECK(vp = m_dl.fetch("FLAC__VERSION_STRING"));
        VERSION_STRING = *reinterpret_cast<char**>(vp);
        CHECK(stream_decoder_new = m_dl.fetch("FLAC__stream_decoder_new"));
        CHECK(stream_decoder_finish =
              m_dl.fetch("FLAC__stream_decoder_finish"));
        CHECK(stream_decoder_delete =
              m_dl.fetch("FLAC__stream_decoder_delete"));
        CHECK(stream_decoder_init_stream =
              m_dl.fetch("FLAC__stream_decoder_init_stream"));
        CHECK(stream_decoder_init_ogg_stream =
              m_dl.fetch("FLAC__stream_decoder_init_ogg_stream"));
        CHECK(stream_decoder_set_metadata_respond =
              m_dl.fetch("FLAC__stream_decoder_set_metadata_respond"));
        CHECK(stream_decoder_process_until_end_of_metadata =
              m_dl.fetch("FLAC__stream_decoder_process_until_end_of_metadata"));
        CHECK(stream_decoder_get_state =
              m_dl.fetch("FLAC__stream_decoder_get_state"));
        CHECK(stream_decoder_process_single =
              m_dl.fetch("FLAC__stream_decoder_process_single"));
        CHECK(stream_decoder_seek_absolute =
              m_dl.fetch("FLAC__stream_decoder_seek_absolute"));
        CHECK(stream_decoder_get_decode_position =
              m_dl.fetch("FLAC__stream_decoder_get_decode_position"));
        CHECK(stream_decoder_reset =
              m_dl.fetch("FLAC__stream_decoder_reset"));
        return true;
    } catch (...) {
        m_dl.reset();
        return false;
    }
}
