#include "CoreAudioEncoder.h"

CoreAudioEncoder::CoreAudioEncoder(AudioConverterX &converter)
    : m_converter(converter),
      m_variable_packet_len(false)
{
    m_converter.getInputStreamDescription(&m_input_desc);
    m_converter.getOutputStreamDescription(&m_output_desc);
    m_stat.setBasicDescription(m_output_desc);
    {
	UInt32 res;
	UInt32 size = sizeof res;
	CHECKCA(AudioFormatGetProperty(
		kAudioFormatProperty_FormatIsExternallyFramed,
		sizeof m_output_desc, &m_output_desc, &size, &res));
        m_requires_packet_desc = !!res;
    }
    {
	AudioBufferList *abl;
	size_t size = offsetof(AudioBufferList, mBuffers[1]);
	abl = static_cast<AudioBufferList*>(std::calloc(1, size));
	abl->mBuffers[0].mNumberChannels = m_output_desc.mChannelsPerFrame;
	abl->mNumberBuffers = 1;
	m_output_abl = x::shared_ptr<AudioBufferList>(abl, std::free);
    }
}

bool CoreAudioEncoder::encodeChunk(UInt32 npackets)
{
    prepareOutputBuffer(npackets);
    AudioBufferList *abl = m_output_abl.get();
    AudioStreamPacketDescription *aspd = &m_packet_desc[0];

#if defined(_MSC_VER) && defined(_DEBUG)
    _CrtCheckMemory();
#endif
    CHECKCA(AudioConverterFillComplexBuffer(m_converter, staticInputDataProc,
		this, &npackets, abl, aspd));
#if defined(_MSC_VER) && defined(_DEBUG)
    _CrtCheckMemory();
#endif

    if (npackets == 0 && abl->mBuffers[0].mDataByteSize == 0)
	return false;

    if (!m_requires_packet_desc) {
	m_sink->writeSamples(abl->mBuffers[0].mData,
		abl->mBuffers[0].mDataByteSize, npackets);
	m_stat.updateWritten(npackets, abl->mBuffers[0].mDataByteSize);
    } else {
	for (uint32_t i = 0; i < npackets; ++i) {
	    if (aspd[i].mVariableFramesInPacket) m_variable_packet_len = true;
	    uint32_t nsamples =
		m_variable_packet_len ? aspd[i].mVariableFramesInPacket
				      : m_output_desc.mFramesPerPacket;
	    if (nsamples) {
		uint8_t *p = static_cast<uint8_t*>(abl->mBuffers[0].mData);
		m_sink->writeSamples(
		    p + aspd[i].mStartOffset, aspd[i].mDataByteSize, nsamples);
		m_stat.updateWritten(nsamples, aspd[i].mDataByteSize);
	    }
	}
    }
    return true;
}

long CoreAudioEncoder::inputDataProc(UInt32 *npackets, AudioBufferList *abl)
{
    prepareInputBuffer(abl, *npackets);
    AudioBuffer &ab = abl->mBuffers[0];
    try {
#if defined(_MSC_VER) && defined(_DEBUG)
	_CrtCheckMemory();
#endif
	*npackets = m_src->readSamples(ab.mData, *npackets);
#if defined(_MSC_VER) && defined(_DEBUG)
	_CrtCheckMemory();
#endif
    } catch (...) {
	return -39; // eofError
    }
    if (*npackets == 0) {
	ab.mData = 0;
	ab.mDataByteSize = 0;
    } else
	ab.mDataByteSize = *npackets * m_input_desc.mBytesPerFrame;
    m_stat.updateRead(*npackets);
    return 0;
}

void CoreAudioEncoder::prepareInputBuffer(AudioBufferList *abl, size_t npackets)
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

void CoreAudioEncoder::prepareOutputBuffer(size_t npackets)
{
    if (m_packet_desc.size() < npackets) {
	m_packet_desc.resize(npackets);
	uint32_t bpp = m_output_desc.mBytesPerPacket;
	if (!bpp) bpp = m_converter.getMaximumOutputPacketSize();
	m_output_buffer.resize(npackets * bpp * 2);
    }
    AudioBufferList *abl = m_output_abl.get();
    abl->mBuffers[0].mDataByteSize = m_output_buffer.size();
    abl->mBuffers[0].mData = &m_output_buffer[0];
}
