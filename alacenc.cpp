#include "alacenc.h"
#include "cautil.h"

ALACEncoderX::ALACEncoderX(const AudioStreamBasicDescription &desc)
    : m_encoder(new ALACEncoder())
{
    m_input_desc.asbd = desc;
    memset(&m_output_desc, 0, sizeof m_output_desc);
    AudioStreamBasicDescription & oasbd = m_output_desc.asbd;
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
    CHECKCA(m_encoder->InitializeEncoder(m_output_desc.afd));

    m_stat.setBasicDescription(oasbd);
    uint32_t pullbytes = desc.mBytesPerFrame * kALACDefaultFramesPerPacket;
    m_input_buffer.resize(pullbytes);
    m_output_buffer.resize(pullbytes * 2);
}

bool ALACEncoderX::encodeChunk(UInt32 npackets)
{
    for (UInt32 i = 0; i < npackets; ++i) {
	size_t nsamples =
	    m_src->readSamples(&m_input_buffer[0], kALACDefaultFramesPerPacket);
	if (nsamples == 0)
	    return false;
	m_stat.updateRead(nsamples);
	int32_t nbytes = nsamples * m_input_desc.asbd.mBytesPerFrame;
	m_encoder->Encode(m_input_desc.afd, m_output_desc.afd,
		&m_input_buffer[0], &m_output_buffer[0], &nbytes);
	m_sink->writeSamples(&m_output_buffer[0], nbytes, nsamples);
	m_stat.updateWritten(nsamples, nbytes);
    }
    return true;
}

void ALACEncoderX::getMagicCookie(std::vector<uint8_t> *cookie)
{
    uint32_t size =
	m_encoder->GetMagicCookieSize(m_output_desc.asbd.mChannelsPerFrame);
    std::vector<uint8_t> vec(size);
    m_encoder->GetMagicCookie(&vec[0], &size);
    cookie->swap(vec);
}
