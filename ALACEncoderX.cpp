#include "ALACEncoderX.h"
#include "cautil.h"

ALACEncoderX::ALACEncoderX(const AudioStreamBasicDescription &desc)
    : m_encoder(new ALACEncoder()), m_iasbd(desc)
{
    std::memcpy(&m_iafd, &desc, sizeof desc);
    m_iafd.mBytesPerFrame =
        ((desc.mBitsPerChannel + 7) & ~7) * desc.mChannelsPerFrame / 8;
    m_iafd.mBytesPerPacket = m_iafd.mBytesPerFrame * m_iafd.mFramesPerPacket;

    memset(&m_odesc, 0, sizeof m_odesc);
    AudioStreamBasicDescription & oasbd = m_odesc.asbd;
    oasbd.mFormatID = kALACFormatAppleLossless;
    if (desc.mFormatFlags & kAudioFormatFlagIsFloat)
        throw std::runtime_error("ALAC: Float PCM is not supported");
    switch (desc.mBitsPerChannel) {
    case 16:
        oasbd.mFormatFlags = 1; break;
    case 20:
        oasbd.mFormatFlags = 2; break;
    case 24:
        oasbd.mFormatFlags = 3; break;
    case 32:
        oasbd.mFormatFlags = 4; break;
    default:
        throw std::runtime_error("ALAC: Not supported bit depth");
    }
    if (desc.mFormatFlags & kAudioFormatFlagIsBigEndian)
        throw std::runtime_error("ALAC: Big endian input is not supported");
    oasbd.mChannelsPerFrame = desc.mChannelsPerFrame;
    oasbd.mSampleRate = desc.mSampleRate;
    oasbd.mFramesPerPacket = kALACDefaultFramesPerPacket;
    CHECKCA(m_encoder->InitializeEncoder(m_odesc.afd));

    m_stat.setBasicDescription(oasbd);
    uint32_t pullbytes = desc.mBytesPerFrame * kALACDefaultFramesPerPacket;
    m_input_buffer.resize(pullbytes);
    m_output_buffer.resize(pullbytes * 2);
}

uint32_t ALACEncoderX::encodeChunk(UInt32 npackets)
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
        m_encoder->Encode(m_iafd, m_odesc.afd, &m_input_buffer[0],
                          &m_output_buffer[0], &xbytes);
        m_sink->writeSamples(&m_output_buffer[0], xbytes, nsamples);
        m_stat.updateWritten(nsamples, xbytes);
    }
    return n;
}

void ALACEncoderX::getMagicCookie(std::vector<uint8_t> *cookie)
{
    uint32_t size =
        m_encoder->GetMagicCookieSize(m_odesc.asbd.mChannelsPerFrame);
    std::vector<uint8_t> vec(size);
    m_encoder->GetMagicCookie(&vec[0], &size);
    cookie->swap(vec);
}
