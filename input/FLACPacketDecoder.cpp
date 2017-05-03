#include <functional>
#include "FLACPacketDecoder.h"
#include "cautil.h"

namespace {
    template <typename T> void try__(T expr, const char *msg)
    {
        if (!expr) throw std::runtime_error(msg);
    }
}
#define TRYFL(expr) (void)(try__((expr), #expr))

FLACPacketDecoder::FLACPacketDecoder(IPacketFeeder *feeder)
    : m_feeder(feeder), m_module(FLACModule::instance())
{
    if (!m_module.loaded()) throw std::runtime_error("libFLAC not loaded");
    memset(&m_iasbd, 0, sizeof(m_iasbd));
    memset(&m_oasbd, 0, sizeof(m_oasbd));

    m_decoder = decoder_t(m_module.stream_decoder_new(),
                          std::bind1st(std::mem_fun(&ThisType::close), this));
    auto st = m_module.stream_decoder_init_stream(m_decoder.get(),
                                                  staticReadCallback,
                                                  staticSeekCallback,
                                                  staticTellCallback,
                                                  staticLengthCallback,
                                                  staticEofCallback,
                                                  staticWriteCallback,
                                                  staticMetadataCallback,
                                                  staticErrorCallback,
                                                  this);
    TRYFL(st == FLAC__STREAM_DECODER_INIT_STATUS_OK);
}

void FLACPacketDecoder::reset()
{
    TRYFL(m_module.stream_decoder_reset(m_decoder.get()));
}

void FLACPacketDecoder::setMagicCookie(const std::vector<uint8_t> &cookie)
{
    m_packet_buffer.reserve(cookie.size() + 4);
    auto p = m_packet_buffer.write_ptr();
    std::memcpy(p, "fLaC", 4);
    std::memcpy(p + 4, cookie.data(), cookie.size());
    m_packet_buffer.commit(cookie.size() + 4);
    auto st =
        m_module.stream_decoder_process_until_end_of_metadata(m_decoder.get());
    TRYFL(st);
}

size_t FLACPacketDecoder::decode(void *data, size_t nsamples)
{
    if (m_feeder->feed(&m_feed_buffer)) {
        m_packet_buffer.reserve(m_feed_buffer.size());
        auto p = m_packet_buffer.write_ptr();
        std::memcpy(p, m_feed_buffer.data(), m_feed_buffer.size());
        m_packet_buffer.commit(m_feed_buffer.size());
        TRYFL(m_module.stream_decoder_process_single(m_decoder.get()));
    }
    nsamples = std::min(nsamples, m_decode_buffer.count());
    if (nsamples)
        std::memcpy(data,
                    m_decode_buffer.read(nsamples),
                    nsamples * m_oasbd.mBytesPerFrame);
    return nsamples;
}

FLAC__StreamDecoderReadStatus
FLACPacketDecoder::readCallback(FLAC__byte *buffer, size_t *bytes)
{
    auto n = std::min(m_packet_buffer.count(), *bytes);
    auto p = m_packet_buffer.read_ptr();
    std::memcpy(buffer, p, n);
    m_packet_buffer.advance(n);
    *bytes = n;
    return n > 0 ? FLAC__STREAM_DECODER_READ_STATUS_CONTINUE
                 : FLAC__STREAM_DECODER_READ_STATUS_END_OF_STREAM;
}

FLAC__StreamDecoderWriteStatus
FLACPacketDecoder::writeCallback(const FLAC__Frame *frame,
                          const FLAC__int32 *const * buffer)
{
    const FLAC__FrameHeader &h = frame->header;
    if (h.channels != m_oasbd.mChannelsPerFrame
     || h.sample_rate != m_oasbd.mSampleRate
     || h.bits_per_sample != m_oasbd.mBitsPerChannel)
        return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;

    /*
     * FLAC sample is aligned to low. We make it aligned to high by
     * shifting to MSB side.
     */
    uint32_t shifts = 32 - h.bits_per_sample;
    m_decode_buffer.reserve(h.blocksize);
    int32_t *bp = m_decode_buffer.write_ptr();
    for (size_t i = 0; i < h.blocksize; ++i)
        for (size_t n = 0; n < h.channels; ++n)
            *bp++ = (buffer[n][i] << shifts);
    m_decode_buffer.commit(h.blocksize);

    return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

void FLACPacketDecoder::metadataCallback(const FLAC__StreamMetadata *metadata)
{
    if (metadata->type == FLAC__METADATA_TYPE_STREAMINFO) {
        auto si = metadata->data.stream_info;
        m_iasbd.mSampleRate = si.sample_rate;
        m_iasbd.mFormatID = 'fLaC';
        if (si.max_blocksize == si.min_blocksize)
            m_iasbd.mFramesPerPacket = si.max_blocksize;
        m_iasbd.mChannelsPerFrame = si.channels;
        m_oasbd = cautil::buildASBDForPCM2(si.sample_rate,
                                           si.channels,
                                           si.bits_per_sample,
                                           32,
                                           kAudioFormatFlagIsSignedInteger);
        m_decode_buffer.set_unit(si.channels);
    }
}

void FLACPacketDecoder::errorCallback(FLAC__StreamDecoderErrorStatus status)
{
}
