#include "VorbisPacketDecoder.h"
#include "cautil.h"
#include "ascutil.h"
#include "VorbisChannelLayout.h"

#define CHECK(expr) do { if (!(expr)) throw std::runtime_error("!?"); } \
    while (0)

bool VorbisModule::load(const std::string &path)
{
    if (!m_dl.load(path))
        return false;
    try {
        CHECK(version_string = m_dl.fetch("vorbis_version_string"));
        CHECK(info_init = m_dl.fetch("vorbis_info_init"));
        CHECK(info_clear = m_dl.fetch("vorbis_info_clear"));
        CHECK(comment_init = m_dl.fetch("vorbis_comment_init"));
        CHECK(comment_clear = m_dl.fetch("vorbis_comment_clear"));
        CHECK(synthesis_headerin = m_dl.fetch("vorbis_synthesis_headerin"));
        CHECK(synthesis_init = m_dl.fetch("vorbis_synthesis_init"));
        CHECK(synthesis_restart = m_dl.fetch("vorbis_synthesis_restart"));
        CHECK(synthesis = m_dl.fetch("vorbis_synthesis"));
        CHECK(synthesis_blockin = m_dl.fetch("vorbis_synthesis_blockin"));
        CHECK(synthesis_pcmout = m_dl.fetch("vorbis_synthesis_pcmout"));
        CHECK(synthesis_read = m_dl.fetch("vorbis_synthesis_read"));
        CHECK(block_init = m_dl.fetch("vorbis_block_init"));
        CHECK(block_clear = m_dl.fetch("vorbis_block_clear"));
        CHECK(dsp_clear = m_dl.fetch("vorbis_dsp_clear"));
        return true;
    } catch (...) {
        m_dl.reset();
        return false;
    }
}

VorbisPacketDecoder::VorbisPacketDecoder()
    : m_module(VorbisModule::instance()), m_headerDone(false)
{
    if (!m_module.loaded()) throw std::runtime_error("libvorbis not loaded");
    memset(&m_iasbd, 0, sizeof(m_iasbd));
    memset(&m_oasbd, 0, sizeof(m_oasbd));
    memset(&m_info, 0, sizeof(m_info));
    memset(&m_comment, 0, sizeof(m_comment));
    memset(&m_dsp, 0, sizeof(m_dsp));
    memset(&m_block, 0, sizeof(m_block));
}

VorbisPacketDecoder::~VorbisPacketDecoder()
{
    if (m_headerDone) {
        m_module.block_clear(&m_block);
        m_module.dsp_clear(&m_dsp);
    }
    m_module.comment_clear(&m_comment);
    m_module.info_clear(&m_info);
}

void VorbisPacketDecoder::reset()
{
    if (m_headerDone)
        m_module.synthesis_restart(&m_dsp);
}

void VorbisPacketDecoder::setMagicCookie(const std::vector<uint8_t> &cookie)
{
    m_module.info_init(&m_info);
    m_module.comment_init(&m_comment);

    const uint8_t *p = cookie.data();
    const uint8_t *end = p + cookie.size();
    for (int i = 0; i < 3; ++i) {
        if (end - p < 4)
            throw std::runtime_error("Malformed Vorbis header packets");
        uint32_t len = (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
        p += 4;
        if (end - p < static_cast<ptrdiff_t>(len))
            throw std::runtime_error("Malformed Vorbis header packets");

        ogg_packet op;
        memset(&op, 0, sizeof op);
        op.packet = const_cast<unsigned char*>(p);
        op.bytes = len;
        op.b_o_s = (i == 0);
        op.packetno = i;
        if (m_module.synthesis_headerin(&m_info, &m_comment, &op) < 0)
            throw std::runtime_error("Invalid Vorbis header packet");
        p += len;
    }

    CHECK(m_module.synthesis_init(&m_dsp, &m_info) == 0);
    CHECK(m_module.block_init(&m_dsp, &m_block) == 0);
    m_headerDone = true;

    m_iasbd.mFormatID = util::fourcc("vorb");
    m_iasbd.mSampleRate = m_info.rate;
    m_iasbd.mChannelsPerFrame = m_info.channels;
    m_oasbd = ascutil::buildASBDForPCM2(m_info.rate, m_info.channels,
                                        32, 32, kAudioFormatFlagIsFloat);
}

size_t VorbisPacketDecoder::decode(const std::vector<uint8_t> &packet,
                                   std::vector<uint8_t> *samples)
{
    ogg_packet op;
    memset(&op, 0, sizeof op);
    op.packet = const_cast<unsigned char*>(packet.data());
    op.bytes = packet.size();
    op.granulepos = -1;

    samples->clear();
    if (m_module.synthesis(&m_block, &op) != 0)
        return 0;
    CHECK(m_module.synthesis_blockin(&m_dsp, &m_block) == 0);

    unsigned channels = m_info.channels;
    const uint8_t *order = vorbis_channel_layout[channels - 1];
    int total = 0;
    float **pcm;
    int n;
    while ((n = m_module.synthesis_pcmout(&m_dsp, &pcm)) > 0) {
        size_t offset = samples->size();
        samples->resize(offset + n * m_oasbd.mBytesPerFrame);
        float *dst = reinterpret_cast<float*>(samples->data() + offset);
        for (int i = 0; i < n; ++i)
            for (unsigned ch = 0; ch < channels; ++ch)
                *dst++ = pcm[order[ch]][i];
        m_module.synthesis_read(&m_dsp, n);
        total += n;
    }
    return total;
}
