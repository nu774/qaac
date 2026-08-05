#include "ALACEncoderX.h"
#include "cautil.h"

namespace {
    AudioFormatDescription toFormatDescription(
            const ca::AudioStreamBasicDescription &x)
    {
        AudioFormatDescription y;
        y.mSampleRate = x.mSampleRate;
        y.mFormatID = x.mFormatID;
        y.mFormatFlags = x.mFormatFlags;
        y.mBytesPerPacket = x.mBytesPerPacket;
        y.mFramesPerPacket = x.mFramesPerPacket;
        y.mBytesPerFrame = x.mBytesPerFrame;
        y.mChannelsPerFrame = x.mChannelsPerFrame;
        y.mBitsPerChannel = x.mBitsPerChannel;
        y.mReserved = x.mReserved;
        return y;
    }
}

ALACEncoderX::ALACEncoderX(const ca::AudioStreamBasicDescription &desc)
    : m_encoder(new ALACEncoder()), m_iasbd(desc)
{
    m_iafd = toFormatDescription(desc);
    m_iafd.mBytesPerFrame =
        ((desc.mBitsPerChannel + 7) & ~7) * desc.mChannelsPerFrame / 8;
    m_iafd.mBytesPerPacket = m_iafd.mBytesPerFrame * m_iafd.mFramesPerPacket;

    memset(&m_oasbd, 0, sizeof m_oasbd);
    m_oasbd.mFormatID = kALACFormatAppleLossless;
    if (desc.mFormatFlags & kAudioFormatFlagIsFloat)
        throw std::runtime_error("ALAC: Float PCM is not supported");
    switch (desc.mBitsPerChannel) {
    case 16:
        m_oasbd.mFormatFlags = 1; break;
    case 20:
        m_oasbd.mFormatFlags = 2; break;
    case 24:
        m_oasbd.mFormatFlags = 3; break;
    case 32:
        m_oasbd.mFormatFlags = 4; break;
    default:
        throw std::runtime_error("ALAC: Not supported bit depth");
    }
    if (desc.mFormatFlags & kAudioFormatFlagIsBigEndian)
        throw std::runtime_error("ALAC: Big endian input is not supported");
    m_oasbd.mChannelsPerFrame = desc.mChannelsPerFrame;
    m_oasbd.mSampleRate = desc.mSampleRate;
    m_oasbd.mFramesPerPacket = kALACDefaultFramesPerPacket;
    m_oafd = toFormatDescription(m_oasbd);
    CHECKCA(m_encoder->InitializeEncoder(m_oafd));

    m_stat.setBasicDescription(m_oasbd);
    uint32_t pullbytes = desc.mBytesPerFrame * kALACDefaultFramesPerPacket;
    m_input_buffer.resize(pullbytes);
    m_output_buffer.resize(pullbytes * 2);
}

uint32_t ALACEncoderX::encodeChunk(uint32_t npackets)
{
    unsigned n = 0;
    for (n = 0; n < npackets; ++n) {
        size_t nsamples = readSamplesFull(src(), &m_input_buffer[0],
                                          kALACDefaultFramesPerPacket);
        if (nsamples == 0)
            break;
        size_t nbytes = nsamples * m_iasbd.mBytesPerFrame;
        if (m_iafd.mBytesPerFrame < m_iasbd.mBytesPerFrame)
            util::pack(&m_input_buffer[0], &nbytes,
                       m_iasbd.mBytesPerFrame / m_iasbd.mChannelsPerFrame,
                       m_iafd.mBytesPerFrame / m_iafd.mChannelsPerFrame);

        int32_t xbytes = static_cast<int32_t>(nbytes);
        m_encoder->Encode(m_iafd, m_oafd, &m_input_buffer[0],
                          &m_output_buffer[0], &xbytes);
        m_sink->writeSamples(&m_output_buffer[0], xbytes, nsamples);
        m_stat.updateWritten(nsamples, xbytes);
    }
    return n;
}

std::vector<uint8_t> ALACEncoderX::getMagicCookie()
{
    uint32_t size =
        m_encoder->GetMagicCookieSize(m_oasbd.mChannelsPerFrame);
    std::vector<uint8_t> vec(size);
    m_encoder->GetMagicCookie(vec.data(), &size);
    return vec;
}

bool ALACEncoderX::isAvailableOutputChannelLayout(uint32_t channel_layout_tag)
{
    switch (channel_layout_tag) {
    case kALACChannelLayoutTag_Mono:
    case kALACChannelLayoutTag_Stereo:
    case kALACChannelLayoutTag_MPEG_3_0_B:
    case kALACChannelLayoutTag_MPEG_4_0_B:
    case kALACChannelLayoutTag_MPEG_5_0_D:
    case kALACChannelLayoutTag_MPEG_5_1_D:
    case kALACChannelLayoutTag_AAC_6_1:
    case kALACChannelLayoutTag_MPEG_7_1_B:
        return true;
    }
    return false;
}
