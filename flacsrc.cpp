#include "flacsrc.h"
#include "utf8_codecvt_facet.hpp"
#include "strcnv.h"
#include "itunetags.h"
#include "cuesheet.h"
#include "win32util.h"

namespace flac {
    template <typename T> void try__(T expr, const char *msg)
    {
	if (!expr) throw std::runtime_error(format("ERROR: %s", msg));
    }

    inline void want(bool expr)
    {
	if (!expr)
	    throw std::runtime_error("Sorry, unacceptable FLAC format");
    }

    void validate(const FLAC__StreamMetadata_StreamInfo &si)
    {
	want(si.sample_rate > 0);
	want(si.channels > 0 && si.channels < 9);
	want((si.bits_per_sample & 7) == 0);
	want(si.bits_per_sample >= 8 && si.bits_per_sample <= 32);
    }
}
#define TRYFL(expr) (void)(flac::try__((expr), #expr))

FLACModule::FLACModule(const std::wstring &path)
{
    HMODULE hDll;
    hDll = LoadLibraryW(path.c_str());
    m_loaded = (hDll != NULL);
    if (!m_loaded)
	return;
    try {
	TRYFL(stream_decoder_new =
		ProcAddress(hDll, "FLAC__stream_decoder_new"));
	TRYFL(stream_decoder_finish =
		ProcAddress(hDll, "FLAC__stream_decoder_finish"));
	TRYFL(stream_decoder_delete =
		ProcAddress(hDll, "FLAC__stream_decoder_delete"));
	TRYFL(stream_decoder_init_stream =
		ProcAddress(hDll, "FLAC__stream_decoder_init_stream"));
	TRYFL(stream_decoder_init_ogg_stream =
		ProcAddress(hDll, "FLAC__stream_decoder_init_ogg_stream"));
	TRYFL(stream_decoder_set_metadata_respond = ProcAddress(hDll,
		    "FLAC__stream_decoder_set_metadata_respond"));
	TRYFL(stream_decoder_process_until_end_of_metadata = ProcAddress(hDll,
		    "FLAC__stream_decoder_process_until_end_of_metadata"));
	TRYFL(stream_decoder_get_state =
		ProcAddress(hDll, "FLAC__stream_decoder_get_state"));
	TRYFL(stream_decoder_process_single =
		ProcAddress(hDll, "FLAC__stream_decoder_process_single"));
	TRYFL(stream_decoder_seek_absolute =
		ProcAddress(hDll, "FLAC__stream_decoder_seek_absolute"));
    } catch (...) {
	FreeLibrary(hDll);
	m_loaded = false;
	return;
    }
    m_module.swap(module_t(hDll, FreeLibrary));
}


FLACSource::FLACSource(const FLACModule &module, InputStream &stream):
    m_module(module),
    m_stream(stream),
    m_giveup(false),
    m_samples_read(0),
    m_duration(-1)
{
    char buffer[33];
    check_eof(m_stream.read(buffer, 33) == 33);
    uint32_t fcc = fourcc(buffer);
    m_stream.pushback(buffer, 33);
    if ((fcc != 'fLaC' && fcc != 'OggS')
     || (fcc == 'OggS' && std::memcmp(&buffer[28], "\177FLAC", 5)))
	throw std::runtime_error("Not a FLAC file");

    m_decoder.swap(decoder_t(m_module.stream_decoder_new(),
		std::bind1st(std::mem_fun(&FLACSource::close_decoder), this)));
    TRYFL(m_module.stream_decoder_set_metadata_respond(
		m_decoder.get(), FLAC__METADATA_TYPE_VORBIS_COMMENT));

    TRYFL((fcc == 'OggS' ? m_module.stream_decoder_init_ogg_stream
			 : m_module.stream_decoder_init_stream)
	    (m_decoder.get(),
	     staticReadCallback,
	     staticSeekCallback,
	     staticTellCallback,
	     staticLengthCallback,
	     staticEofCallback,
	     staticWriteCallback,
	     staticMetadataCallback,
	     staticErrorCallback,
	     this) == FLAC__STREAM_DECODER_INIT_STATUS_OK);
    TRYFL(m_module.stream_decoder_process_until_end_of_metadata(
		m_decoder.get()));
    if (m_giveup || m_format.m_bitsPerSample == 0)
	flac::want(false);
    m_buffer.resize(m_format.m_nchannels);
    if (m_cuesheet.size()) {
	try {
	    CueSheetToChapters(m_cuesheet, m_format.m_rate,
		    m_duration, &m_chapters);
	} catch (...) {}
    }
}

void FLACSource::setRange(int64_t start, int64_t length)
{
    int64_t dur = static_cast<int64_t>(m_duration);
    if (length >= 0 && (dur == -1 || length < dur))
	m_duration = length;
    if (start)
	TRYFL(m_module.stream_decoder_seek_absolute(m_decoder.get(), start));
    if (start > 0 && dur > 0 && length == -1)
	m_duration -= start;
}

template <class MemorySink>
size_t FLACSource::readSamplesT(void *buffer, size_t nsamples)
{
    uint64_t rest = m_duration - m_samples_read;
    nsamples = static_cast<size_t>(
	    std::min(static_cast<uint64_t>(nsamples), rest));
    if (!nsamples) return 0;
    MemorySink sink(buffer);
    size_t processed = 0;
    while (processed < nsamples) {
	while (m_buffer[0].size() && processed < nsamples) {
	    for (size_t i = 0; i < m_buffer.size(); ++i) {
		sink.put(m_buffer[i].front());
		m_buffer[i].pop_front();
	    }
	    ++processed;
	}
	if (processed < nsamples) {
	    if (m_giveup)
		break;
	    if (m_module.stream_decoder_get_state(m_decoder.get()) ==
		    FLAC__STREAM_DECODER_END_OF_STREAM)
		break;
	    TRYFL(m_module.stream_decoder_process_single(m_decoder.get()));
	}
    }
    m_samples_read += processed;
    return processed;
}

size_t FLACSource::readSamples(void *buffer, size_t nsamples)
{
    if (m_format.m_bitsPerSample == 8)
	return readSamplesT<MemorySink8>(buffer, nsamples);
    else if (m_format.m_bitsPerSample == 16)
	return readSamplesT<MemorySink16LE>(buffer, nsamples);
    else if (m_format.m_bitsPerSample == 24)
	return readSamplesT<MemorySink24LE>(buffer, nsamples);
    else
	return readSamplesT<MemorySink32LE>(buffer, nsamples);
}

FLAC__StreamDecoderReadStatus
FLACSource::readCallback(FLAC__byte *buffer, unsigned *bytes)
{
    *bytes = m_stream.read(buffer, *bytes);
    return *bytes == 0 ? FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM
		       : FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
}

FLAC__StreamDecoderSeekStatus
FLACSource::seekCallback(uint64_t offset)
{
    if (!m_stream.seekable())
	return FLAC__STREAM_DECODER_SEEK_STATUS_UNSUPPORTED; 
    int64_t rc = m_stream.seek(offset, ISeekable::kBegin);
    if (rc == offset)
	return FLAC__STREAM_DECODER_SEEK_STATUS_OK; 
    else
	return FLAC__STREAM_DECODER_SEEK_STATUS_ERROR; 
}

FLAC__StreamDecoderTellStatus
FLACSource::tellCallback(uint64_t *offset)
{
    *offset = m_stream.tell();
    return *offset == -1 ? FLAC__STREAM_DECODER_TELL_STATUS_ERROR
			 : FLAC__STREAM_DECODER_TELL_STATUS_OK; 
}

FLAC__StreamDecoderLengthStatus
FLACSource::lengthCallback(uint64_t *length)
{
    if (!m_stream.seekable())
	return FLAC__STREAM_DECODER_LENGTH_STATUS_UNSUPPORTED;
    *length = m_stream.size();
    return *length == -1 ? FLAC__STREAM_DECODER_LENGTH_STATUS_ERROR
			 : FLAC__STREAM_DECODER_LENGTH_STATUS_OK; 
}

FLAC__bool FLACSource::eofCallback()
{
    char ch;
    if (m_stream.read(&ch, 1) != 1)
	return true;
    m_stream.pushback(ch);
    return false;
}

FLAC__StreamDecoderWriteStatus
FLACSource::writeCallback(
	const FLAC__Frame *frame, const FLAC__int32 *const * buffer)
{
    const FLAC__FrameHeader &h = frame->header;
    if (h.channels != m_format.m_nchannels
     || h.sample_rate != m_format.m_rate
     || h.bits_per_sample != m_format.m_bitsPerSample)
	return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;

    for (size_t i = 0; i < h.channels; ++i)
	std::copy(buffer[i], buffer[i] + h.blocksize,
		std::back_inserter(m_buffer[i]));

    return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

void FLACSource::metadataCallback(const FLAC__StreamMetadata *metadata)
{
    if (metadata->type == FLAC__METADATA_TYPE_STREAMINFO)
	handleStreamInfo(metadata->data.stream_info);
    else if (metadata->type == FLAC__METADATA_TYPE_VORBIS_COMMENT)
	handleVorbisComment(metadata->data.vorbis_comment);
}

void FLACSource::errorCallback(FLAC__StreamDecoderErrorStatus status)
{
    m_giveup = true;
}

void FLACSource::handleStreamInfo(const FLAC__StreamMetadata_StreamInfo &si)
{
    try {
	flac::validate(si);
    } catch (const std::runtime_error) {
	m_giveup = true;
	return;
    }
    m_duration = si.total_samples;
    m_format.m_type = SampleFormat::kIsSignedInteger;
    m_format.m_bitsPerSample = si.bits_per_sample;
    m_format.m_endian = SampleFormat::kIsLittleEndian;
    m_format.m_nchannels = si.channels;
    m_format.m_rate = si.sample_rate;
}

void FLACSource::handleVorbisComment(
	const FLAC__StreamMetadata_VorbisComment &vc)
{
    for (size_t i = 0; i < vc.num_comments; ++i) {
	const char *cs = reinterpret_cast<const char *>(vc.comments[i].entry);
	std::pair<uint32_t, std::wstring> kv;
	std::string key;
	if (TransVorbisComment(cs, &kv, &key))
	    m_tags.insert(kv);
	else if (!strcasecmp(key.c_str(), "cuesheet"))
	    m_cuesheet = kv.second;
    }
}
