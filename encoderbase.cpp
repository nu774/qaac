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

EncoderBase::EncoderBase(const x::shared_ptr<ISource> &src,
    uint32_t formatID, int nchannelsOut, int chanmask) :
	m_src(src),
	m_samples_read(0),
	m_frames_written(0),
	m_bytes_written(0),
	m_max_bitrate(0),
	m_cur_bitrate(0)
{
    m_input_desc = BuildBasicDescription(src->getSampleFormat());
    setInputBasicDescription(m_input_desc);
    uint32_t nchannels = m_input_desc.mChannelsPerFrame;
    const std::vector<uint32_t> *chanmap = src->getChannelMap();
    AudioChannelLayoutX layout;
    if (chanmask > 0)
	layout = AudioChannelLayoutX::FromBitmap(chanmask);
    else if (chanmask == 0 || !chanmap)
	layout = AudioChannelLayoutX::CreateDefault(nchannels);
    else
	layout = AudioChannelLayoutX::FromChannelMap(*chanmap);
    setInputChannelLayout(layout);

    uint32_t newtag = GetAACChannelMap(layout, 0);
    if (newtag) {
	layout->mChannelLayoutTag = newtag;
	layout->mChannelBitmap = 0;
    }
    /*
     * Basically, channel layout of output is automatically selected by QT.
     * However, this "default" behavior seems to take
     * only number of channels into account.
     * (For example, even if input is C L R Cs, L R Ls Rs is selected).
     * Therefore, we must explicitly reset output layout here.
     */
    if (formatID != 'alac' && nchannels == nchannelsOut) {
	if (formatID == 'aach') {
	    const static uint32_t supported[] = {
		kAudioChannelLayoutTag_Mono,
		kAudioChannelLayoutTag_Stereo,
		kAudioChannelLayoutTag_Quadraphonic,
		kAudioChannelLayoutTag_AAC_5_1,
		kAudioChannelLayoutTag_AAC_7_1
	    };
	    const uint32_t *endp = supported + array_size(supported);
	    if (std::find(supported, endp, layout->mChannelLayoutTag) == endp)
		throw std::runtime_error("Not supported channel layout for HE");
	}
	setChannelLayout(layout);
    }
    if (formatID == 'alac' && nchannelsOut != 2)
	throw std::runtime_error("Only stereo is supported for ALAC");
    AudioStreamBasicDescription oasbd = { 0 };
    oasbd.mChannelsPerFrame = nchannelsOut;
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
	uint32_t nsamples = (m_output_desc.mFormatID == 'alac')
	    ? aspd[i].mVariableFramesInPacket
	    : m_output_desc.mFramesPerPacket;
	if (nsamples) {
	    m_sink->writeSamples(
		reinterpret_cast<char*>(ab.mData) + aspd[i].mStartOffset,
		aspd[i].mDataByteSize,
		m_output_desc.mFormatID == 'aach' ? nsamples / 2 : nsamples);
	    double rate = calcBitrate(aspd[i].mDataByteSize, nsamples);
	    if (rate > m_max_bitrate)
		m_max_bitrate = rate;
	}
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
