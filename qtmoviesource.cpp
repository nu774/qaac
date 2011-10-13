#include "qtmoviesource.h"
#include "qtmoviehelper.h"
#include "itunetags.h"
#include "strcnv.h"

static
Track GetSoundTrack(Movie movie)
{
    size_t count = GetMovieTrackCount(movie);
    for (size_t i = 0; i < count; ++i) {
	Media media = GetTrackMedia(GetMovieIndTrack(movie, i + 1));
	OSType type;
	GetMediaHandlerDescription(media, &type, 0, 0);
	if (type == SoundMediaType)
	    return GetMediaTrack(media);
    }
    throw std::runtime_error("Movie contains no sound track");
}

static
void DisableOtherTracks(Movie movie, Track track)
{
    long id = GetTrackID(track);
    size_t count = GetMovieTrackCount(movie);
    for (size_t i = 0; i < count; ++i) {
	track = GetMovieIndTrack(movie, i + 1);
	if (GetTrackID(track) != id)
	    SetTrackEnabled(track, false);
    }
}

static SampleFormat
ConvertFromBasicDescription(const AudioStreamBasicDescription &asbd)
{
    SampleFormat format;
    if (asbd.mFormatFlags & kAudioFormatFlagIsSignedInteger)
	format.m_type = SampleFormat::kIsSignedInteger;
    else if (asbd.mFormatFlags & kAudioFormatFlagIsFloat)
	format.m_type = SampleFormat::kIsFloat;
    else
	format.m_type = SampleFormat::kIsUnsignedInteger;

    if (asbd.mFormatFlags & kAudioFormatFlagIsBigEndian)
	format.m_endian = SampleFormat::kIsBigEndian;
    else
	format.m_endian = SampleFormat::kIsLittleEndian;

    format.m_nchannels = asbd.mChannelsPerFrame;
    format.m_rate = static_cast<unsigned>(asbd.mSampleRate);
    format.m_bitsPerSample = asbd.mBitsPerChannel;
    return format;
}

QTMovieSource::QTMovieSource(const std::wstring &path)
    : m_extraction_complete(false)
{
    MovieX movie = MovieX::FromFile(path);
    Track track = GetSoundTrack(movie);
    DisableOtherTracks(movie, track);
    /*
     *  In order to get sample accurate extraction,
     *	we have to set movie timescale := media timescale (= sample rate)
     */
    DeleteTrackSegment(track, 0, GetTrackDuration(track));
    Media media = GetTrackMedia(track);
    SetMovieTimeScale(movie, GetMediaTimeScale(media));
    InsertMediaIntoTrack(track, 0, 0, GetMediaDuration(media), fixed1);

    SoundDescriptionX sd;
    GetMediaSampleDescription(media, 1,
	    reinterpret_cast<SampleDescriptionHandle>(
		static_cast<SoundDescriptionHandle>(sd)));
    AudioStreamBasicDescription asbd;
    sd.getAudioStreamBasicDescription(&asbd);
    if (asbd.mFormatID != 'lpcm' && asbd.mFormatID != 'alac')
	throw std::runtime_error("QTMovieSource: Not supported input format");

    AudioChannelLayoutX layout, layoutm;
    TrackX(track).getChannelLayout(&layout);
    movie.getSummaryChannelLayout(&layoutm);

    if (asbd.mFormatID == 'alac') {
	uint32_t tag = layoutm->mChannelLayoutTag;
	if (layoutm.numChannels() != asbd.mChannelsPerFrame)
	    tag = GetALACLayoutTag(asbd.mChannelsPerFrame);
	std::vector<uint32_t> cmap;
	uint32_t bitmap = GetAACReversedChannelMap(tag, &cmap);
	for (size_t i = 0; i < cmap.size(); ++i)
	    m_channel_conversion_map.push_back(cmap[i] - 1);
	for (size_t i = 1; i <= 32; ++i, bitmap >>= 1)
	    if (bitmap & 1) m_chanmap.push_back(i);
	M4ATagParser parser(GetFullPathNameX(path.c_str()));
	m_tags = parser.getTags();
    } else {
	if (layoutm->mNumberChannelDescriptions == asbd.mChannelsPerFrame)
	    for (size_t i = 0; i < asbd.mChannelsPerFrame; ++i)
		m_chanmap.push_back(
			layoutm->mChannelDescriptions[i].mChannelLabel);
    }
    m_session = MovieAudioExtractionX::Begin(movie, 0);
    m_session.setRenderQuality(kQTAudioRenderQuality_Max);
    m_session.setAllChannelsDiscrete(true);
    m_session.getAudioStreamBasicDescription(&m_description);

    /*
     * MovieAudioExtraction produces non-interleaved PCM by default.
     */
    m_description.mFormatFlags &= ~kLinearPCMFormatFlagIsNonInterleaved;
    /*
     * cf. "scaudiocompress" sample by Apple.
     * MovieAudioExtraction produces 32bit float PCM by default.
     */
    UInt32 mss = m_session.getMaxSampleSize();  // "Real" sample size.
    if (mss > 0 && mss <= 32) {
	m_description.mBitsPerChannel = mss;
	m_description.mFormatFlags &= ~kLinearPCMFormatFlagIsFloat;
	m_description.mFormatFlags |= kLinearPCMFormatFlagIsSignedInteger;
	m_description.mBytesPerFrame
	    = m_description.mBytesPerPacket
	    = (mss >> 3) * m_description.mChannelsPerFrame;
    }
    m_session.setAudioStreamBasicDescription(m_description);
    m_format = ConvertFromBasicDescription(m_description);

    setRange(0, static_cast<uint64_t>(GetMediaDuration(media))
       	* m_description.mSampleRate / GetMediaTimeScale(media));
}

void QTMovieSource::skipSamples(int64_t count)
{
    TimeRecord tr;
    m_session.getCurrentTime(&tr);
    tr.value.hi += static_cast<SInt32>(count >> 32);
    tr.value.lo += static_cast<SInt32>(count);
    m_session.setCurrentTime(tr);
}

size_t QTMovieSource::readSamples(void *buffer, size_t nsamples)
{
    nsamples = adjustSamplesToRead(nsamples);
    if (!nsamples || m_extraction_complete)
	return 0;

    UInt32 flags = 0;
    /*
     * We've told MovieAudioExtraction to produce Interleaved samples,
     * so number of buffers is 1
     */

    AudioBufferList abl = {1, {{ 0 }}};
    AudioBuffer &ab = abl.mBuffers[0];
    ab.mNumberChannels = m_description.mChannelsPerFrame;
    ab.mData = buffer;
    ab.mDataByteSize = nsamples * m_description.mBytesPerFrame;
    UInt32 npackets = nsamples;
    flags = 0;
    TRYE(MovieAudioExtractionFillBuffer(m_session, &npackets, &abl, &flags));
#if 1
    if (m_channel_conversion_map.size() && npackets) {
	size_t width = m_description.mBitsPerChannel >> 3;
	size_t framelen = m_description.mBytesPerFrame;
	std::vector<char> tmp(framelen);
	char *bp = reinterpret_cast<char*>(buffer);
	const uint32_t *cmap = &m_channel_conversion_map[0];
	for (size_t i = 0; i < npackets ; ++i, bp += framelen) {
	    std::memcpy(&tmp[0], bp, framelen);
	    for (size_t j = 0; j < m_chanmap.size(); ++j) {
		std::memcpy(bp + width * j, &tmp[0] + width * cmap[j], width);
	    }
	}
    }
#endif
    if (flags & kQTMovieAudioExtractionComplete)
	m_extraction_complete = true;
    addSamplesRead(npackets);
    return npackets;
}
