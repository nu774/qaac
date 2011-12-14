#include "afsource.h"
#include "win32util.h"
#include "CoreAudioHelper.h"

namespace audiofile {
    const int ioErr = -36;

    OSStatus read(void *cookie, SInt64 pos, UInt32 count, void *data,
	    UInt32 *nread)
    {
	InputStream *pT = reinterpret_cast<InputStream*>(cookie);
	if (pT->seek(pos, ISeekable::kBegin) != pos)
	    return ioErr;
	*nread = pT->read(data, count);
	return 0;
    }
    SInt64 size(void *cookie)
    {
	InputStream *pT = reinterpret_cast<InputStream*>(cookie);
	return pT->size();
    }
    OSStatus setsize(void *cookie, SInt64 size)
    {
	return ioErr;
    }
    OSStatus write(void *cookie, SInt64 pos, UInt32 count, const void *data,
	    UInt32 *nwritten)
    {
	return ioErr;
    }
}

#if 0
AFSource::AFSource(const wchar_t *path)
    : m_offset(0)
{
    /*
     * XXX:
     * UNC path seems correctly handled, but very long file name is not,
     * with or without \\?\ prefix.
     */
    CFStringPtr sp = W2CF(path);
    CFURLRef url = CFURLCreateWithFileSystemPath(0, sp.get(),
	    kCFURLWindowsPathStyle, false);
    x::shared_ptr<const __CFURL> urlPtr(url, CFRelease);
    AudioFileID afid;
    CHECKCA(AudioFileOpenURL(url, kAudioFileReadPermission, 0, &afid));
    m_af.attach(afid, true);
    init();
}
#endif

AFSource::AFSource(InputStream &stream)
    : m_offset(0), m_stream(stream)
{
    AudioFileID afid;
    CHECKCA(AudioFileOpenWithCallbacks(
		&m_stream, audiofile::read, audiofile::write,
		audiofile::size, audiofile::setsize, 0, &afid));
    m_af.attach(afid, true);
    init();
}

size_t AFSource::readSamples(void *buffer, size_t nsamples)
{
    nsamples = adjustSamplesToRead(nsamples);
    if (!nsamples) return 0;
    UInt32 ns = nsamples;
    UInt32 nb = ns * m_format.bytesPerFrame();
    CHECKCA(AudioFileReadPackets(m_af, false, &nb, 0,
		m_offset + getSamplesRead(), &ns, buffer));
    addSamplesRead(ns);
    return ns;
}

void AFSource::init()
{
    //fourcc fmt(m_af.getFileFormat());
    AudioStreamBasicDescription asbd;
    m_af.getDataFormat(&asbd);
    if (asbd.mFormatID != 'lpcm')
	throw std::runtime_error("AFSource: not supported format");
    m_format.m_nchannels = asbd.mChannelsPerFrame;
    m_format.m_bitsPerSample = asbd.mBitsPerChannel;
    m_format.m_rate = asbd.mSampleRate;
    if (asbd.mFormatFlags & kAudioFormatFlagIsFloat)
	m_format.m_type = SampleFormat::kIsFloat;
    else if (asbd.mFormatFlags & kAudioFormatFlagIsSignedInteger)
	m_format.m_type = SampleFormat::kIsSignedInteger;
    else
	m_format.m_type = SampleFormat::kIsUnsignedInteger;
    if (asbd.mFormatFlags & kAudioFormatFlagIsBigEndian)
	m_format.m_endian = SampleFormat::kIsBigEndian;
    else
	m_format.m_endian = SampleFormat::kIsLittleEndian;
    setRange(0, m_af.getAudioDataPacketCount());
    x::shared_ptr<AudioChannelLayout> acl;
    try {
	m_af.getChannelLayout(&acl);
	uint32_t bitmap = GetBitmapFromAudioChannelLayout(acl.get());
	if (bitmap && bitcount(bitmap) >= m_format.m_nchannels) {
	    for (size_t i = 0;
		 m_chanmap.size() < m_format.m_nchannels && i < 32;
		 ++i, bitmap >>= 1)
		if (bitmap & 1) m_chanmap.push_back(i + 1);
	}
    } catch (std::exception &) {}
}
