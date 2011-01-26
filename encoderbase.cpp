#include "encoderbase.h"
#include <crtdbg.h>

static 
AudioStreamBasicDescription BuildBasicDescription(const SampleFormat &format)
{
    AudioStreamBasicDescription desc = { 0 };
    desc.mFormatID = kAudioFormatLinearPCM;
    desc.mFormatFlags = kAudioFormatFlagIsPacked;
    if (format.m_type == SampleFormat::kIsSignedInteger)
	desc.mFormatFlags |= kAudioFormatFlagIsSignedInteger;
    else if (format.m_type == SampleFormat::kIsFloat)
	desc.mFormatFlags |= kAudioFormatFlagIsFloat;
    if (format.m_endian == SampleFormat::kIsBigEndian)
	desc.mFormatFlags |= kAudioFormatFlagIsBigEndian;
    desc.mFramesPerPacket = 1;
    desc.mChannelsPerFrame = format.m_nchannels;
    desc.mSampleRate = format.m_rate;
    desc.mBitsPerChannel = format.m_bitsPerSample;
    desc.mBytesPerPacket
	= desc.mBytesPerFrame
	= format.m_nchannels * format.m_bitsPerSample >> 3;
    return desc;
}

EncoderBase::EncoderBase(ISource *src, uint32_t formatID) :
	m_src(src),
	m_sink(0),
	m_samples_read(0),
	m_frames_written(0),
	m_bytes_written(0),
	m_max_bitrate(0),
	m_cur_bitrate(0)
{
    m_input_desc = BuildBasicDescription(src->getSampleFormat());
    setInputBasicDescription(m_input_desc);
    uint32_t nchannels = m_input_desc.mChannelsPerFrame;
    if (formatID == 'alac') {
	if (nchannels != 2)
	    throw std::runtime_error(
		"Multi-channel ALAC encoding is currently not supported");
	else if (m_input_desc.mBitsPerChannel != 16 &&
		 m_input_desc.mBitsPerChannel != 24)
	    throw std::runtime_error(
		"Only 16/24bit format is supported for ALAC encoding");
    }
    const std::vector<uint32_t> *chanmap = src->getChannelMap();
    AudioChannelLayoutX layout =
	chanmap ? AudioChannelLayoutX::FromChannelMap(*chanmap)
	        : AudioChannelLayoutX::CreateBasic(nchannels);
    setInputChannelLayout(layout);

    AudioStreamBasicDescription oasbd = { 0 };
    oasbd.mChannelsPerFrame = (nchannels == 3) ? 2 : nchannels;
    oasbd.mFormatID = formatID;
    setBasicDescription(oasbd);
    getBasicDescription(&m_output_desc);
}

bool EncoderBase::encodeChunk(UInt32 nframes)
{
    prepareOutputBuffer(nframes);
    AudioStreamPacketDescription *aspd = &m_packet_desc[0];

    AudioBufferList abl = { 1, 0 };
    AudioBuffer &ab = abl.mBuffers[0];
    ab.mNumberChannels = m_output_desc.mChannelsPerFrame;
    ab.mData = &m_output_buffer[0];
    ab.mDataByteSize = m_output_buffer.size();

#ifdef _DEBUG
    _CrtCheckMemory();
#endif
    TRYE(SCAudioFillBuffer(
		*this, staticInputDataProc, this, &nframes, &abl, aspd));
#ifdef _DEBUG
    _CrtCheckMemory();
#endif

    if (nframes == 0 && ab.mDataByteSize == 0)
	return false;

    uint64_t wbytes = 0, wsamples = 0;
    for (uint32_t i = 0; i < nframes; ++i) {
	uint32_t nsamples = aspd[i].mVariableFramesInPacket
	    ? aspd[i].mVariableFramesInPacket
	    : m_output_desc.mFramesPerPacket;
	m_sink->writeSamples(
		reinterpret_cast<char*>(ab.mData) + aspd[i].mStartOffset,
		aspd[i].mDataByteSize,
		m_output_desc.mFormatID == 'aach' ? nsamples / 2 : nsamples);
	double rate = calcBitrate(aspd[i].mDataByteSize, nsamples);
	if (rate > m_max_bitrate)
	    m_max_bitrate = rate;
	wbytes += aspd[i].mDataByteSize;
	wsamples += nsamples;
    }
    m_cur_bitrate = calcBitrate(wbytes, wsamples);
    m_frames_written += nframes;
    m_bytes_written += wbytes;
    return true;
}

long EncoderBase::inputDataProc(UInt32 *nframes, AudioBufferList *abl)
{
    /*
     * We deal with interleaved PCM only,
     * therefore abl->mNumberBuffers will be always equal to 1.
     */
    prepareInputBuffer(abl, *nframes);
#ifdef _DEBUG
    _CrtCheckMemory();
#endif
    try {
	*nframes = m_src->readSamples(abl->mBuffers[0].mData, *nframes);
    } catch (...) {
	return eofErr;
    }
#ifdef _DEBUG
    _CrtCheckMemory();
#endif
    if (*nframes == 0) {
	// Tell SCAudioFillBuffer() that we are done.
	abl->mBuffers[0].mData = 0;
	abl->mBuffers[0].mDataByteSize = 0;
    } else
        abl->mBuffers[0].mDataByteSize =
	    *nframes * m_input_desc.mBytesPerFrame;
	m_samples_read += *nframes;
    return 0;
}

void EncoderBase::prepareOutputBuffer(size_t nframes)
{
    if (m_packet_desc.size() < nframes) {
	m_packet_desc.resize(nframes);
	getBasicDescription(&m_output_desc);
	uint32_t bpp = m_output_desc.mBytesPerPacket;
	if (!bpp)
	    bpp = getMaximumOutputPacketSize();
//	m_output_buffer.resize(nframes * bpp);
	/*
	 * XXX
	 * ScAudioFillBuffer() randomly causes heap corruption
	 * when ALAC encoding, for some sources.
	 * Making bigger buffer seems to work.
	 */
	m_output_buffer.resize(nframes * bpp * 2);
    }
}

void EncoderBase::prepareInputBuffer(AudioBufferList *abl, size_t nframe)
{
    size_t size = nframe * m_input_desc.mBytesPerFrame;
    if (abl->mBuffers[0].mData &&
	abl->mBuffers[0].mDataByteSize >= size)
	return;
    m_input_buffer.resize(abl->mNumberBuffers * size);
    char *p = &m_input_buffer[0];
    for (size_t i = 0; i < abl->mNumberBuffers; ++i, p += size) {
	abl->mBuffers[i].mDataByteSize = size;
	abl->mBuffers[i].mData = p;
    }
}
