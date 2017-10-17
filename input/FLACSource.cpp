#include <functional>
#include "FLACSource.h"
#include "strutil.h"
#include "metadata.h"
#include "cautil.h"
#include "win32util.h"
#include "chanmap.h"

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

FLACSource::FLACSource(const std::shared_ptr<FILE> &fp):
    m_eof(false),
    m_giveup(false),
    m_initialize_done(false),
    m_length(0),
    m_position(0),
    m_fp(fp),
    m_module(FLACModule::instance())
{
    if (!m_module.loaded()) throw std::runtime_error("libFLAC not loaded");
    char buffer[33];
    util::check_eof(util::nread(fileno(m_fp.get()), buffer, 33) == 33);
    if (std::memcmp(buffer, "ID3", 3) == 0) {
        uint32_t size = 0;
        for (int i = 6; i < 10; ++i) {
            size <<= 7;
            size |= buffer[i];
        }
        CHECKCRT(_lseeki64(fileno(m_fp.get()), 10 + size, SEEK_SET) < 0);
        util::check_eof(util::nread(fileno(m_fp.get()), buffer, 33) == 33);
    }
    uint32_t fcc = util::fourcc(buffer);
    if ((fcc != 'fLaC' && fcc != 'OggS')
     || (fcc == 'OggS' && std::memcmp(&buffer[28], "\177FLAC", 5)))
        throw std::runtime_error("Not a FLAC file");
    CHECKCRT(_lseeki64(fileno(m_fp.get()), 0, SEEK_SET) < 0);

    m_decoder =
        decoder_t(m_module.stream_decoder_new(),
                  std::bind1st(std::mem_fun(&FLACSource::close), this));
    TRYFL(m_module.stream_decoder_set_metadata_respond(
                m_decoder.get(), FLAC__METADATA_TYPE_VORBIS_COMMENT));
    TRYFL(m_module.stream_decoder_set_metadata_respond(
                m_decoder.get(), FLAC__METADATA_TYPE_PICTURE));

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
    m_buffer.set_unit(m_asbd.mChannelsPerFrame);
    m_initialize_done = true;
}

void FLACSource::seekTo(int64_t count)
{
    if (count == m_position)
        return;
    m_buffer.reset();
    TRYFL(m_module.stream_decoder_seek_absolute(m_decoder.get(), count));
    m_position = count;
}

size_t FLACSource::readSamples(void *buffer, size_t nsamples)
{
    if (m_giveup)
        throw std::runtime_error("FLAC decoder error");
    if (!m_buffer.count()) {
        if (m_module.stream_decoder_get_state(m_decoder.get()) ==
                FLAC__STREAM_DECODER_END_OF_STREAM)
            return 0;
        TRYFL(m_module.stream_decoder_process_single(m_decoder.get()));
    }
    uint32_t count = std::min(m_buffer.count(), nsamples);
    if (count) {
        uint32_t bytes = count * m_asbd.mChannelsPerFrame * 4;
        std::memcpy(buffer, m_buffer.read(count), bytes);
        m_position += count;
    }
    return count;
}

FLAC__StreamDecoderReadStatus
FLACSource::readCallback(FLAC__byte *buffer, size_t *bytes)
{
    ssize_t n = util::nread(fileno(m_fp.get()), buffer, *bytes);
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

    /*
     * FLAC sample is aligned to low. We make it aligned to high by
     * shifting to MSB side.
     */
    uint32_t shifts = 32 - h.bits_per_sample;
    m_buffer.reserve(h.blocksize);
    int32_t *bp = m_buffer.write_ptr();
    for (size_t i = 0; i < h.blocksize; ++i)
        for (size_t n = 0; n < h.channels; ++n)
            *bp++ = (buffer[n][i] << shifts);
    m_buffer.commit(h.blocksize);

    return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

void FLACSource::metadataCallback(const FLAC__StreamMetadata *metadata)
{
    if (m_initialize_done)
        return;
    if (metadata->type == FLAC__METADATA_TYPE_STREAMINFO)
        handleStreamInfo(metadata->data.stream_info);
    else if (metadata->type == FLAC__METADATA_TYPE_VORBIS_COMMENT)
        handleVorbisComment(metadata->data.vorbis_comment);
    else if (metadata->type == FLAC__METADATA_TYPE_PICTURE)
        handlePicture(metadata->data.picture);
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
    m_length = si.total_samples;
    m_asbd = cautil::buildASBDForPCM2(si.sample_rate, si.channels,
                                      si.bits_per_sample, 32,
                                      kAudioFormatFlagIsSignedInteger);
}

void FLACSource::handleVorbisComment(
        const FLAC__StreamMetadata_VorbisComment &vc)
{
    std::map<std::string, std::string> tags(m_tags);
    for (size_t i = 0; i < vc.num_comments; ++i) {
        const char *cs = reinterpret_cast<const char *>(vc.comments[i].entry);
        strutil::Tokenizer<char> tokens(cs, "=");
        char *key = tokens.next();
        char *value = tokens.rest();
        if (strcasecmp(key, "waveformatextensible_channel_mask") == 0) {
            unsigned mask = 0;
            if (sscanf(value, "%i", &mask) == 1)
                m_chanmap = chanmap::getChannels(mask);
        } else if (value) {
            tags[key] = value;
        }
    }
    m_tags = TextBasedTag::normalizeTags(tags);
}

void FLACSource::handlePicture(const FLAC__StreamMetadata_Picture &pic)
{
    if (pic.type == FLAC__STREAM_METADATA_PICTURE_TYPE_FRONT_COVER)
        m_tags["COVER ART"] = std::string(pic.data, pic.data + pic.data_length);
}
