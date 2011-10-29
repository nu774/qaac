#include "SCAudioEncoder.h"

SCAudioEncoder::SCAudioEncoder(StdAudioComponentX &scaudio): m_scaudio(scaudio)
{
    m_scaudio.getInputBasicDescription(&m_input_desc);
    m_scaudio.getBasicDescription(&m_output_desc);
    m_stat.setBasicDescription(m_output_desc);

    if (m_input_desc.mFormatFlags & kLinearPCMFormatFlagIsNonInterleaved)
	throw std::runtime_error(
		"non interleaved format is not supported");
    if (m_output_desc.mFormatFlags & kLinearPCMFormatFlagIsNonInterleaved)
	throw std::runtime_error(
		"non interleaved format is not supported");

    AudioBufferList *abl;
    size_t size = offsetof(AudioBufferList, mBuffers[1]);
    abl = static_cast<AudioBufferList*>(std::calloc(1, size));
    abl->mBuffers[0].mNumberChannels = m_output_desc.mChannelsPerFrame;
    abl->mNumberBuffers = 1;
    m_output_abl = x::shared_ptr<AudioBufferList>(abl, std::free);
}

bool SCAudioEncoder::encodeChunk(UInt32 npackets)
{
    prepareOutputBuffer(npackets);
    AudioBufferList *abl = m_output_abl.get();
    AudioStreamPacketDescription *aspd = &m_packet_desc[0];

    TRYE(SCAudioFillBuffer(m_scaudio, staticInputDataProc,
		this, &npackets, abl, aspd));

    if (npackets == 0 && abl->mBuffers[0].mDataByteSize == 0)
	return false;

    for (uint32_t i = 0; i < npackets; ++i) {
	uint32_t nsamples = (m_output_desc.mFormatID == 'alac')
	    ? aspd[i].mVariableFramesInPacket
	    : m_output_desc.mFramesPerPacket;
	if (nsamples) {
	    uint8_t *p = static_cast<uint8_t*>(abl->mBuffers[0].mData);
	    m_sink->writeSamples(
		p + aspd[i].mStartOffset, aspd[i].mDataByteSize, nsamples);
	    m_stat.updateWritten(nsamples, aspd[i].mDataByteSize);
	}
    }
    return true;
}

long SCAudioEncoder::inputDataProc(UInt32 *npackets, AudioBufferList *abl)
{
    prepareInputBuffer(abl, *npackets);
    AudioBuffer &ab = abl->mBuffers[0];
    try {
	*npackets = m_src->readSamples(ab.mData, *npackets);
    } catch (...) {
	return eofErr;
    }
    if (*npackets == 0) {
	ab.mData = 0;
	ab.mDataByteSize = 0;
    } else
	ab.mDataByteSize = *npackets * m_input_desc.mBytesPerFrame;
    m_stat.updateRead(*npackets);
    return 0;
}

void SCAudioEncoder::prepareInputBuffer(AudioBufferList *abl, size_t npackets)
{
    size_t size = npackets * m_input_desc.mBytesPerFrame;
    if (abl->mBuffers[0].mData &&
	abl->mBuffers[0].mDataByteSize >= size)
	return;
    m_input_buffer.resize(abl->mNumberBuffers * size);
    uint8_t *p = &m_input_buffer[0];
    for (size_t i = 0; i < abl->mNumberBuffers; ++i, p += size) {
	abl->mBuffers[i].mDataByteSize = size;
	abl->mBuffers[i].mData = p;
    }
}

void SCAudioEncoder::prepareOutputBuffer(size_t npackets)
{
    if (m_packet_desc.size() < npackets) {
	m_packet_desc.resize(npackets);
	uint32_t bpp = m_output_desc.mBytesPerPacket;
	if (!bpp) bpp = m_scaudio.getMaximumOutputPacketSize();
	m_output_buffer.resize(npackets * bpp * 2);
    }
    AudioBufferList *abl = m_output_abl.get();
    abl->mBuffers[0].mDataByteSize = m_output_buffer.size();
    abl->mBuffers[0].mData = &m_output_buffer[0];
}
