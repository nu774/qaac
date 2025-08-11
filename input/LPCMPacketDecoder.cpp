#include "LPCMPacketDecoder.h"
#include "cautil.h"

LPCMPacketDecoder::LPCMPacketDecoder()
{
    std::memset(&m_iasbd, 0, sizeof m_iasbd);
    std::memset(&m_oasbd, 0, sizeof m_oasbd);
}

void LPCMPacketDecoder::setMagicCookie(const std::vector<uint8_t> &cookie)
{
    std::memcpy(&m_iasbd, cookie.data(), cookie.size());
    if (m_iasbd.mFormatFlags & kAudioFormatFlagIsFloat) {
        m_oasbd = cautil::buildASBDForPCM2(m_iasbd.mSampleRate, m_iasbd.mChannelsPerFrame,
            m_iasbd.mBitsPerChannel, m_iasbd.mBitsPerChannel,
            kAudioFormatFlagIsFloat);
    } else {
        m_oasbd = cautil::buildASBDForPCM2(m_iasbd.mSampleRate, m_iasbd.mChannelsPerFrame,
            m_iasbd.mBitsPerChannel, 32,
            kAudioFormatFlagIsSignedInteger);
    }
}

size_t LPCMPacketDecoder::decode(const std::vector<uint8_t> &packet, std::vector<uint8_t> *samples)
{
    m_pivot.resize(packet.size());
    std::memcpy(m_pivot.data(), packet.data(), packet.size());
    if ((m_iasbd.mFormatFlags & kAudioFormatFlagIsBigEndian)) {
        util::bswapbuffer(m_pivot.data(), m_pivot.size(), (m_iasbd.mBitsPerChannel + 7) & ~7);
    }
    samples->resize(m_pivot.size() * m_oasbd.mBytesPerFrame / m_iasbd.mBytesPerFrame);
    size_t size = m_pivot.size();
    util::unpack(m_pivot.data(), samples->data(), &size,
                    m_iasbd.mBytesPerFrame / m_iasbd.mChannelsPerFrame,
                    m_oasbd.mBytesPerFrame / m_oasbd.mChannelsPerFrame);
    if (!(m_iasbd.mFormatFlags & kAudioFormatFlagIsFloat) && !(m_iasbd.mFormatFlags & kAudioFormatFlagIsSignedInteger)) {
        util::convert_sign(reinterpret_cast<uint32_t*>(samples->data()), samples->size() / sizeof(int32_t));
    }
    return size / m_oasbd.mBytesPerFrame;
}
