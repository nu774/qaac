#include "flacsink.h"
#include "strcnv.h"
#include "utf8_codecvt_facet.hpp"
#include "itunetags.h"

namespace flac {
    template <typename T> void try__(T expr, const char *msg)
    {
	if (!expr) throw std::runtime_error(format("ERROR: %s", msg));
    }
}
#define TRYFL(expr) (void)(flac::try__((expr), #expr))

FLACSink::FLACSink(FILE *fp, uint64_t duration, const SampleFormat &fmt,
		   const FLACModule &module,
		   const std::map<uint32_t, std::wstring> &tags)
    :m_fp(fp), m_module(module), m_format(fmt)
{
    if (fmt.m_type == SampleFormat::kIsFloat)
	throw std::runtime_error("Can't handle float source");
    if (fmt.m_endian == SampleFormat::kIsBigEndian)
	throw std::runtime_error("Can't handle big endian source");
    m_encoder.swap(encoder_t(m_module.stream_encoder_new(),
		std::bind1st(std::mem_fun(&FLACSink::closeEncoder), this)));
    TRYFL(m_module.stream_encoder_set_channels(
		m_encoder.get(), fmt.m_nchannels));
    TRYFL(m_module.stream_encoder_set_bits_per_sample(
		m_encoder.get(), fmt.m_bitsPerSample));
    TRYFL(m_module.stream_encoder_set_sample_rate(
		m_encoder.get(), fmt.m_rate));
    TRYFL(m_module.stream_encoder_set_total_samples_estimate(
		m_encoder.get(), duration));
    TRYFL(m_module.stream_encoder_set_compression_level(
		m_encoder.get(), 5));

    m_meta[0] =
	m_module.metadata_object_new(FLAC__METADATA_TYPE_VORBIS_COMMENT);
    m_meta[1] = m_module.metadata_object_new(FLAC__METADATA_TYPE_PADDING);
    m_meta[1]->length = 0x1000;
    {
	FLAC__StreamMetadata_VorbisComment_Entry entry;
	std::map<uint32_t, std::wstring>::const_iterator ii;
	utf8_codecvt_facet u8codec;
	for (ii = tags.begin(); ii != tags.end(); ++ii) {
	    const char *key = GetNameFromTagID(ii->first);
	    if (!key) continue;
	    std::string value = w2m(ii->second, u8codec);
	    m_module.metadata_object_vorbiscomment_entry_from_name_value_pair(
		    &entry, key, value.c_str());
	    m_module.metadata_object_vorbiscomment_append_comment(
		    m_meta[0], entry, false);
	}
    }
    m_module.stream_encoder_set_metadata(m_encoder.get(), m_meta, 2);
    
    TRYFL(m_module.stream_encoder_init_stream(m_encoder.get(),
		staticWriteCallback, 0, 0, 0, this)
		    == FLAC__STREAM_ENCODER_INIT_STATUS_OK);
} 

void FLACSink::writeSamples(const void *data, size_t length, size_t nsamples)
{
    std::vector<int32_t> buff;
    switch (m_format.m_bitsPerSample) {
    case 8:
	{
	    const char *src = reinterpret_cast<const char *>(data);
	    for (size_t i = 0; i < length; ++i)
		buff.push_back(src[i]);
	}
	break;
    case 16:
	{
	    const int16_t *src = reinterpret_cast<const int16_t*>(data);
	    for (size_t i = 0; i < length / 2; ++i)
		buff.push_back(src[i]);
	}
	break;
    case 24:
	{
	    const uint8_t *src = reinterpret_cast<const uint8_t*>(data);
	    for (size_t i = 0; i < length / 3; ++i)
		buff.push_back(src[i*3]|(src[i*3+1] << 8)|(src[i*3+2] << 16));
	}
	break;
    case 32:
	{
	    const int32_t *src = reinterpret_cast<const int32_t *>(data);
	    for (size_t i = 0; i < length / 4; ++i)
		buff.push_back(src[i]);
	}
	break;
    }
    TRYFL(m_module.stream_encoder_process_interleaved(
		m_encoder.get(), &buff[0], nsamples));
}
