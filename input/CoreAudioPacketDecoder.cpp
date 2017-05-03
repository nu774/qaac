#include "CoreAudioPacketDecoder.h"
#include "cautil.h"

CoreAudioPacketDecoder::
CoreAudioPacketDecoder(IPacketFeeder *feeder,
                       const AudioStreamBasicDescription & asbd)
    : m_iasbd(asbd), m_feeder(feeder)
{
    bool is_float = false;
    int valid_bits;

    switch (asbd.mFormatID) {
    case 'alac':
        switch (asbd.mFormatFlags) {
        case 1: valid_bits = 16; break;
        case 2: valid_bits = 20; break;
        case 3: valid_bits = 24; break;
        case 4: valid_bits = 32; break;
        }
        break;
    case 'aac ':
    case 'aach':
    case 'aacp':
    case '.mp1':
    case '.mp2':
    case '.mp3':
        valid_bits = 32;
        is_float = true;
        break;
    default:
        throw std::runtime_error("Not supported input codec");
    }
    m_oasbd =
        cautil::buildASBDForPCM2(asbd.mSampleRate,
                                 asbd.mChannelsPerFrame,
                                 valid_bits,
                                 is_float ? valid_bits : 32,
                                 is_float ? kAudioFormatFlagIsFloat
                                          : kAudioFormatFlagIsSignedInteger);
    m_converter = AudioConverterX(m_iasbd, m_oasbd);
    AudioConverterPrimeInfo pinfo = { 0 };
    m_converter.setPrimeInfo(pinfo);
#if 0
    UInt32 method = m_converter.getPrimeMethod();
    m_converter.setPrimeMethod(kConverterPrimeMethod_None);
#endif
}

size_t CoreAudioPacketDecoder::decode(void *data, size_t nsamples)
{
    UInt32 npackets  = nsamples;
    AudioBufferList abl;
    abl.mNumberBuffers  = 1;
    abl.mBuffers[0].mNumberChannels = m_oasbd.mChannelsPerFrame;
    abl.mBuffers[0].mData           = data;
    abl.mBuffers[0].mDataByteSize   = nsamples * m_oasbd.mBytesPerFrame;
    CHECKCA(AudioConverterFillComplexBuffer(m_converter,
                                            staticInputDataProc,
                                            this, &npackets, &abl,
                                            nullptr));
    return npackets;
}

long CoreAudioPacketDecoder::inputDataProc(UInt32 *npackets,
                                           AudioBufferList *abl,
                                           AudioStreamPacketDescription **aspd)
{
    if (m_feeder->feed(&m_packet_buffer)) {
        abl->mBuffers[0].mData         = m_packet_buffer.data();
        abl->mBuffers[0].mDataByteSize = m_packet_buffer.size();
        m_aspd.mDataByteSize           = m_packet_buffer.size();
        *npackets                      = 1;
    } else {
        abl->mBuffers[0].mData         = nullptr;
        abl->mBuffers[0].mDataByteSize = 0;
        m_aspd.mDataByteSize           = 0;
        *npackets                      = 0;
    }
    m_aspd.mStartOffset = 0;
    m_aspd.mVariableFramesInPacket = 0;
    *aspd = &m_aspd;
    return 0;
}
