#include "flacsrc.h"
#include "strutil.h"
#include "itunetags.h"
#include "cuesheet.h"
#include "cautil.h"
#include "win32util.h"

namespace flac {
    template <typename T> void try__(T expr, const char *msg)
    {
	if (!expr) throw std::runtime_error(msg);
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
	want(si.bits_per_sample >= 8 && si.bits_per_sample <= 32);
    }
}
#define TRYFL(expr) (void)(flac::try__((expr), #expr))

FLACSource::FLACSource(const FLACModule &module,
		       const std::shared_ptr<FILE> &fp):
    m_module(module),
    m_fp(fp),
    m_eof(false),
    m_giveup(false)
{
    char buffer[33];
    util::check_eof(read(fileno(m_fp.get()), buffer, 33) == 33);
    uint32_t fcc = util::fourcc(buffer);
    if ((fcc != 'fLaC' && fcc != 'OggS')
     || (fcc == 'OggS' && std::memcmp(&buffer[28], "\177FLAC", 5)))
	throw std::runtime_error("Not a FLAC file");
    _lseeki64(fileno(m_fp.get()), 0, SEEK_SET);

    m_decoder =
	decoder_t(m_module.stream_decoder_new(),
		  std::bind1st(std::mem_fun(&FLACSource::close), this));
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
    if (m_giveup || m_asbd.mBitsPerChannel == 0)
	flac::want(false);
    m_buffer.resize(m_asbd.mChannelsPerFrame);
}

void FLACSource::skipSamples(int64_t count)
{
    TRYFL(m_module.stream_decoder_seek_absolute(m_decoder.get(), count));
}

template <class MemorySink>
size_t FLACSource::readSamplesT(void *buffer, size_t nsamples)
{
    nsamples = adjustSamplesToRead(nsamples);
    if (!nsamples) return 0;
    MemorySink sink(buffer);
    size_t processed = 0;
    uint32_t shifts = (8 - m_asbd.mBitsPerChannel) % 8;
    while (processed < nsamples) {
	while (m_buffer[0].size() && processed < nsamples) {
	    for (size_t i = 0; i < m_buffer.size(); ++i) {
		int value = m_buffer[i].front();
		m_buffer[i].pop_front();
		// FLAC samples are aligned to low.
		if (shifts)
		    value <<= shifts;
		sink.put(value);
	    }
	    ++processed;
	}
	if (processed < nsamples) {
	    if (m_giveup)
		throw std::runtime_error("FLAC decoder error");
	    if (m_module.stream_decoder_get_state(m_decoder.get()) ==
		    FLAC__STREAM_DECODER_END_OF_STREAM)
		break;
	    TRYFL(m_module.stream_decoder_process_single(m_decoder.get()));
	}
    }
    addSamplesRead(processed);
    return processed;
}

size_t FLACSource::readSamples(void *buffer, size_t nsamples)
{
    uint32_t bytesPerChannel =
	m_asbd.mBytesPerFrame / m_asbd.mChannelsPerFrame;
    if (bytesPerChannel == 1)
	return readSamplesT<util::MemorySink8>(buffer, nsamples);
    else if (bytesPerChannel == 2)
	return readSamplesT<util::MemorySink16LE>(buffer, nsamples);
    else if (bytesPerChannel == 3)
	return readSamplesT<util::MemorySink24LE>(buffer, nsamples);
    else
	return readSamplesT<util::MemorySink32LE>(buffer, nsamples);
}

FLAC__StreamDecoderReadStatus
FLACSource::readCallback(FLAC__byte *buffer, size_t *bytes)
{
    ssize_t n = read(fileno(m_fp.get()), buffer, *bytes);
    if (n <= 0) {
	m_eof = true;
	return FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;
    }
    *bytes = n;
    return FLAC__STREAM_DECODER_READ_STATUS_CONTINUE;
}

FLAC__StreamDecoderSeekStatus
FLACSource::seekCallback(uint64_t offset)
{
    m_eof = false;
    if (_lseeki64(fileno(m_fp.get()), offset, SEEK_SET) == offset)
	return FLAC__STREAM_DECODER_SEEK_STATUS_OK; 
    else
	return FLAC__STREAM_DECODER_SEEK_STATUS_ERROR; 
}

FLAC__StreamDecoderTellStatus
FLACSource::tellCallback(uint64_t *offset)
{
    int64_t off = _lseeki64(fileno(m_fp.get()), 0, SEEK_CUR);
    if (off < 0)
	return FLAC__STREAM_DECODER_TELL_STATUS_ERROR;
    *offset = off;
    return FLAC__STREAM_DECODER_TELL_STATUS_OK;
}

FLAC__StreamDecoderLengthStatus
FLACSource::lengthCallback(uint64_t *length)
{
    int64_t len = _filelengthi64(fileno(m_fp.get()));
    if (len < 0)
	return FLAC__STREAM_DECODER_LENGTH_STATUS_ERROR;
    *length = len;
    return FLAC__STREAM_DECODER_LENGTH_STATUS_OK;
}

FLAC__bool FLACSource::eofCallback()
{
    return m_eof;
}

FLAC__StreamDecoderWriteStatus
FLACSource::writeCallback( const FLAC__Frame *frame,
			   const FLAC__int32 *const * buffer)
{
    const FLAC__FrameHeader &h = frame->header;
    if (h.channels != m_asbd.mChannelsPerFrame
     || h.sample_rate != m_asbd.mSampleRate
     || h.bits_per_sample != m_asbd.mBitsPerChannel)
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
    setRange(0, si.total_samples);
    m_asbd = cautil::buildASBDForPCM(si.sample_rate, si.channels,
				si.bits_per_sample,
				kAudioFormatFlagIsSignedInteger,
				kAudioFormatFlagIsAlignedHigh);
}

void FLACSource::handleVorbisComment(
	const FLAC__StreamMetadata_VorbisComment &vc)
{
    std::map<std::string, std::string> vorbisComments;
    std::wstring cuesheet;
    for (size_t i = 0; i < vc.num_comments; ++i) {
	const char *cs = reinterpret_cast<const char *>(vc.comments[i].entry);
	strutil::Tokenizer<char> tokens(cs, "=");
	char *key = tokens.next();
	char *value = tokens.rest();
	if (value) {
	    vorbisComments[key] = value;
	    if (!strcasecmp(key, "cuesheet"))
		cuesheet = strutil::us2w(value);
	    else
		vorbisComments[key] = value;
	}
    }
    Vorbis::ConvertToItunesTags(vorbisComments, &m_tags);
    if (cuesheet.size()) {
	std::map<uint32_t, std::wstring> tags;
	Cue::CueSheetToChapters(cuesheet,
				getDuration() / m_asbd.mSampleRate,
				&m_chapters, &tags);
	std::map<uint32_t, std::wstring>::const_iterator it;
	for (it = tags.begin(); it != tags.end(); ++it)
	    m_tags[it->first] = it->second;
    }
}
