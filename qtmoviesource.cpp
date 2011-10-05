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
    /*
     * By default, Movie's time scale is not accurate enough, and
     * MovieAudioExtraction produces fewer samples than the original,
     * even if it is just a raw PCM source.
     * So, we must reset the time scale to get accurate extraction.
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

    if (asbd.mFormatID == 'alac') {
	M4ATagParser parser(GetFullPathNameX(path.c_str()));
	m_tags = parser.getTags();
    }
    m_session = MovieAudioExtractionX::Begin(movie, 0);
    m_session.getAudioStreamBasicDescription(&m_description);
    m_session.getAudioChannelLayout(&m_layout);

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
    for (size_t i = 0; i < m_layout->mNumberChannelDescriptions; ++i)
	m_chanmap.push_back(m_layout->mChannelDescriptions[i].mChannelLabel);

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
    if (flags & kQTMovieAudioExtractionComplete)
	m_extraction_complete = true;
    addSamplesRead(npackets);
    return npackets;
}
