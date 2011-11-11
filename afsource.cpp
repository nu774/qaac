#include "afsource.h"
#include "win32util.h"
#include "CoreAudioHelper.h"

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
	if (acl->mNumberChannelDescriptions == m_format.m_nchannels) {
	    for (size_t i = 0; i < m_format.m_nchannels; ++i)
		m_chanmap.push_back(
		    acl->mChannelDescriptions[i].mChannelLabel);
	} else {
	    unsigned bitmap = acl->mChannelBitmap;
	    if (!bitmap) {
		switch (acl->mChannelLayoutTag) {
		case kAudioChannelLayoutTag_Quadraphonic:
		    bitmap = 0x33; break;
		case kAudioChannelLayoutTag_MPEG_4_0_A:
		    bitmap = 0x107; break;
		case kAudioChannelLayoutTag_MPEG_5_0_A:
		    bitmap = 0x37; break;
		case kAudioChannelLayoutTag_MPEG_5_1_A:
		    bitmap = 0x3f; break;
		case kAudioChannelLayoutTag_MPEG_6_1_A:
		    bitmap = 0x13f; break;
		case kAudioChannelLayoutTag_MPEG_7_1_A:
		    bitmap = 0xff; break;
		}
	    }
	    if (bitmap) {
		for (size_t i = 0; i < 32; ++i, bitmap >>= 1)
		    if (bitmap & 1) m_chanmap.push_back(i + 1);
	    }
	}
    } catch (std::exception &) {}
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

void AFSource::skipSamples(int64_t count)
{
    m_offset += count;
}
