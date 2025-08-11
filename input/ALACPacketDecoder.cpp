#include "ALACPacketDecoder.h"
#include <ALACBitUtilities.h>
#include "cautil.h"

ALACPacketDecoder::ALACPacketDecoder(const AudioStreamBasicDescription &asbd)
    : m_iasbd(asbd)
{
    int valid_bits;

    switch (asbd.mFormatFlags) {
    case 1: valid_bits = 16; break;
    case 2: valid_bits = 20; break;
    case 3: valid_bits = 24; break;
    case 4: valid_bits = 32; break;
    }
    m_oasbd = cautil::buildASBDForPCM2(asbd.mSampleRate,
                                       asbd.mChannelsPerFrame, valid_bits, 32,
                                       kAudioFormatFlagIsSignedInteger);
    m_decoder = std::make_shared<ALACDecoder>();
    uint32_t bpf =
        (m_oasbd.mBitsPerChannel + 7) / 8 * m_oasbd.mChannelsPerFrame;
    m_raw_decode_buffer.resize(bpf * asbd.mFramesPerPacket);
}

size_t ALACPacketDecoder::decode(const std::vector<uint8_t> &packet, std::vector<uint8_t> *samples)
{
    BitBuffer bits;
    BitBufferInit(&bits, const_cast<uint8_t*>(packet.data()), packet.size());
    uint32_t ncount;
    int err;
    if ((err = m_decoder->Decode(&bits, m_raw_decode_buffer.data(),
        m_iasbd.mFramesPerPacket,
        m_iasbd.mChannelsPerFrame,
        &ncount)) != 0) {
        throw std::runtime_error(strutil::format("ALACDecoder: decode error: %d", err));
    }
    uint32_t bpf =
        (m_oasbd.mBitsPerChannel + 7) / 8 * m_oasbd.mChannelsPerFrame;
    size_t nbytes = ncount * bpf;
    samples->resize(ncount * m_oasbd.mBytesPerFrame);
    util::unpack(m_raw_decode_buffer.data(), samples->data(),
                    &nbytes, bpf / m_oasbd.mChannelsPerFrame, 4);
    return ncount;
}

