#include "OpusPacketDecoder.h"
#include "cautil.h"

#define CHECK(expr) do { if (!(expr)) throw std::runtime_error("!?"); } \
    while (0)

// cf. https://xiph.org/vorbis/doc/Vorbis_I_spec.html#x1-810004.3.9
const uint8_t vorbis_channel_layout[8][8] = {
    { 0 },
    { 0, 1 },
    { 0, 2, 1 },
    { 0, 1, 2, 3 },
    { 0, 2, 1, 3, 4 },
    { 0, 2, 1, 5, 3, 4 },
    { 0, 2, 1, 6, 5, 3, 4 },
    { 0, 2, 1, 7, 5, 6, 3, 4 },
};

bool LibOpusModule::load(const std::wstring &path)
{
    if (!m_dl.load(path))
        return false;
    try {
        CHECK(get_version_string = m_dl.fetch("opus_get_version_string"));
        CHECK(strerror = m_dl.fetch("opus_strerror"));
        CHECK(packet_get_nb_samples = m_dl.fetch("opus_packet_get_nb_samples"));
        CHECK(multistream_decoder_create = m_dl.fetch("opus_multistream_decoder_create"));
        CHECK(multistream_decode_float = m_dl.fetch("opus_multistream_decode_float"));
        CHECK(multistream_decoder_ctl = m_dl.fetch("opus_multistream_decoder_ctl"));
        CHECK(multistream_decoder_destroy = m_dl.fetch("opus_multistream_decoder_destroy"));
        return true;
    } catch (...) {
        m_dl.reset();
        return false;
    }
}

OpusPacketDecoder::OpusPacketDecoder(IPacketFeeder *feeder)
    : m_module(LibOpusModule::instance())
    , m_feeder(feeder)
{
    if (!m_module.loaded()) throw std::runtime_error("libopus not loaded");
    memset(&m_iasbd, 0, sizeof(m_iasbd));
    memset(&m_oasbd, 0, sizeof(m_oasbd));
}

OpusPacketDecoder::~OpusPacketDecoder()
{
}

void OpusPacketDecoder::reset()
{
    if (m_decoder) {
        m_module.multistream_decoder_ctl(m_decoder.get(), OPUS_RESET_STATE);
        m_packet_buffer.clear();
        m_decode_buffer.reset();
    }
}

void OpusPacketDecoder::setMagicCookie(const std::vector<uint8_t> &cookie)
{
    int channel_count = 0;
    unsigned pre_skip = 0;
    opus_uint32 input_sample_rate = 0;
    int output_gain = 0;
    int mapping_family = 0;
    int stream_count = 0;
    int coupled_count = 0;
    unsigned char mapping[256] = { 0 };
    if (cookie.size() >= 11) {
        channel_count = cookie[1];
        pre_skip = (cookie[2]<<8|cookie[3]);
        input_sample_rate = (cookie[4]<<24|cookie[5]<<16|cookie[6]<<8|cookie[7]);
        output_gain = (cookie[8]<<8|cookie[9]);
        mapping_family = cookie[10];
        if (mapping_family > 0 && cookie.size() >= 13 + channel_count) {
            stream_count = cookie[11];
            coupled_count = cookie[12];
            if (mapping_family == 1 && channel_count <= 8) {
                for (int i = 0; i < channel_count; ++i) {
                    mapping[i] = cookie[13 + vorbis_channel_layout[channel_count-1][i]];
                }
            } else {
                memcpy(mapping, &cookie[13], channel_count);
            }
        } else {
            stream_count = 1;
            coupled_count = channel_count - 1;
            mapping[0] = 0;
            mapping[1] = 1;
        }
    }
    if (channel_count && input_sample_rate) {
        int err;
        OpusMSDecoder *decoder = m_module.multistream_decoder_create(48000,
            channel_count, stream_count, coupled_count, mapping, &err);
        if (err) throw std::runtime_error(m_module.strerror(err));
        m_decoder = std::shared_ptr<OpusMSDecoder>(decoder, m_module.multistream_decoder_destroy);
        m_iasbd.mFormatID = util::fourcc("opus");
        m_iasbd.mSampleRate = 48000;
        m_iasbd.mChannelsPerFrame = channel_count;
        m_oasbd = cautil::buildASBDForPCM2(48000, channel_count, 32, 32, kAudioFormatFlagIsFloat);
    }
}

size_t OpusPacketDecoder::decode(void *data, size_t nsamples)
{
    if (m_decoder && m_feeder->feed(&m_packet_buffer)) {
        int fpp = m_module.packet_get_nb_samples(m_packet_buffer.data(), m_packet_buffer.size(), 48000);
        m_decode_buffer.reserve(fpp * m_iasbd.mChannelsPerFrame);
        int nc = m_module.multistream_decode_float(m_decoder.get(),
            m_packet_buffer.data(),
            m_packet_buffer.size(),
            m_decode_buffer.write_ptr(), fpp * m_iasbd.mChannelsPerFrame, 0);
        m_decode_buffer.commit(nc * m_iasbd.mChannelsPerFrame);
        nsamples = std::min(nsamples, (size_t)(m_decode_buffer.count() / m_iasbd.mChannelsPerFrame));
        std::memcpy(data, m_decode_buffer.read_ptr(), nsamples * m_iasbd.mChannelsPerFrame * sizeof(float));
        m_decode_buffer.advance(nsamples * m_iasbd.mChannelsPerFrame);
        return nsamples;
    }
    return 0;
}
