#include "ALACPacketDecoder.h"
#include <ALACBitUtilities.h>
#include "cautil.h"

ALACPacketDecoder::ALACPacketDecoder(IPacketFeeder *feeder,
                                     const AudioStreamBasicDescription &asbd)
    : m_feeder(feeder), m_iasbd(asbd)
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
    m_decode_buffer.set_unit(asbd.mChannelsPerFrame);
}

size_t ALACPacketDecoder::decode(void *data, size_t nsamples)
{
    if (m_feeder->feed(&m_packet_buffer)) {
        BitBuffer bits;
        BitBufferInit(&bits, m_packet_buffer.data(), m_packet_buffer.size());
        uint32_t ncount;
        CHECKCA(m_decoder->Decode(&bits, m_raw_decode_buffer.data(),
                                  m_iasbd.mFramesPerPacket,
                                  m_iasbd.mChannelsPerFrame,
                                  &ncount));
        uint32_t bpf =
            (m_oasbd.mBitsPerChannel + 7) / 8 * m_oasbd.mChannelsPerFrame;
        size_t nbytes = ncount * bpf;
        m_decode_buffer.reserve(ncount);
        util::unpack(m_raw_decode_buffer.data(), m_decode_buffer.write_ptr(),
                     &nbytes, bpf / m_oasbd.mChannelsPerFrame, 4);
        m_decode_buffer.commit(ncount);
    }
    nsamples = std::min(nsamples, m_decode_buffer.count());
    if (nsamples)
        std::memcpy(data, m_decode_buffer.read(nsamples),
                    nsamples * m_oasbd.mBytesPerFrame);
    return nsamples;
}

