#include "win32util.h"
#include "flacmodule.h"

#define CHECK(expr) do { if (!(expr)) throw std::runtime_error("ERROR"); } \
    while (0)

FLACModule::FLACModule(const std::wstring &path)
{
    HMODULE hDll;
    hDll = LoadLibraryW(path.c_str());
    m_loaded = (hDll != NULL);
    if (!m_loaded)
	return;
    try {
	void *vp;
	CHECK(vp = ProcAddress(hDll, "FLAC__VERSION_STRING"));
	VERSION_STRING = *reinterpret_cast<char**>(vp);
	CHECK(stream_decoder_new =
		ProcAddress(hDll, "FLAC__stream_decoder_new"));
	CHECK(stream_decoder_finish =
		ProcAddress(hDll, "FLAC__stream_decoder_finish"));
	CHECK(stream_decoder_delete =
		ProcAddress(hDll, "FLAC__stream_decoder_delete"));
	CHECK(stream_decoder_init_stream =
		ProcAddress(hDll, "FLAC__stream_decoder_init_stream"));
	CHECK(stream_decoder_init_ogg_stream =
		ProcAddress(hDll, "FLAC__stream_decoder_init_ogg_stream"));
	CHECK(stream_decoder_set_metadata_respond = ProcAddress(hDll,
		    "FLAC__stream_decoder_set_metadata_respond"));
	CHECK(stream_decoder_process_until_end_of_metadata = ProcAddress(hDll,
		    "FLAC__stream_decoder_process_until_end_of_metadata"));
	CHECK(stream_decoder_get_state =
		ProcAddress(hDll, "FLAC__stream_decoder_get_state"));
	CHECK(stream_decoder_process_single =
		ProcAddress(hDll, "FLAC__stream_decoder_process_single"));
	CHECK(stream_decoder_seek_absolute =
		ProcAddress(hDll, "FLAC__stream_decoder_seek_absolute"));

	CHECK(stream_encoder_new =
		ProcAddress(hDll, "FLAC__stream_encoder_new"));
	CHECK(stream_encoder_delete =
		ProcAddress(hDll, "FLAC__stream_encoder_delete"));
	CHECK(stream_encoder_set_channels =
		ProcAddress(hDll, "FLAC__stream_encoder_set_channels"));
	CHECK(stream_encoder_set_bits_per_sample =
		ProcAddress(hDll, "FLAC__stream_encoder_set_bits_per_sample"));
	CHECK(stream_encoder_set_sample_rate =
		ProcAddress(hDll, "FLAC__stream_encoder_set_sample_rate"));
	CHECK(stream_encoder_set_total_samples_estimate = ProcAddress(hDll,
		    "FLAC__stream_encoder_set_total_samples_estimate"));
	CHECK(stream_encoder_set_metadata =
		ProcAddress(hDll, "FLAC__stream_encoder_set_metadata"));
	CHECK(stream_encoder_get_state =
		ProcAddress(hDll, "FLAC__stream_encoder_get_state"));
	CHECK(stream_encoder_set_compression_level = ProcAddress(hDll,
		    "FLAC__stream_encoder_set_compression_level"));
	CHECK(stream_encoder_init_stream =
		ProcAddress(hDll, "FLAC__stream_encoder_init_stream"));
	CHECK(stream_encoder_finish =
		ProcAddress(hDll, "FLAC__stream_encoder_finish"));
	CHECK(stream_encoder_process_interleaved =
		ProcAddress(hDll, "FLAC__stream_encoder_process_interleaved"));

	CHECK(metadata_object_new =
		ProcAddress(hDll, "FLAC__metadata_object_new"));
	CHECK(metadata_object_delete =
		ProcAddress(hDll, "FLAC__metadata_object_delete"));
	CHECK(metadata_object_vorbiscomment_append_comment = ProcAddress(hDll,
		    "FLAC__metadata_object_vorbiscomment_append_comment"));
	CHECK(metadata_object_vorbiscomment_entry_from_name_value_pair =
	    ProcAddress(hDll,
	    "FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair"));

    } catch (...) {
	FreeLibrary(hDll);
	m_loaded = false;
	return;
    }
    m_module = module_t(hDll, FreeLibrary);
}
