#include <algorithm>
#include "utf8_codecvt_facet.hpp"
#include "strcnv.h"
#include "sink.h"
#if defined(_MSC_VER) || defined(__MINGW32__)
#include <io.h>
#include <fcntl.h>
#endif

static
uint32_t getSamplingRateIndex(uint32_t rate)
{
    static const uint32_t rtab[] = {
	96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050, 
	16000, 12000, 11025, 8000, 7350
    };
    const uint32_t * const endp = rtab + array_size(rtab);

    const uint32_t *pos = std::find(rtab, endp, rate);
    return pos < endp ? pos - rtab : 0xf;
}

static
uint32_t getChannelConfig(const AudioChannelLayout *layout)
{
    switch (layout->mChannelLayoutTag) {
	case kAudioChannelLayoutTag_Mono: return 1;
	case kAudioChannelLayoutTag_Stereo: return 2;
	case kAudioChannelLayoutTag_AAC_3_0: return 3;
	case kAudioChannelLayoutTag_AAC_4_0: return 4;
	case kAudioChannelLayoutTag_AAC_5_0: return 5;
	case kAudioChannelLayoutTag_AAC_5_1: return 6;
	case kAudioChannelLayoutTag_AAC_7_1: return 7;
    }
    return 0;
}

using mp4v2::impl::MP4Atom;

MP4SinkBase::MP4SinkBase(const std::wstring &path, bool temp)
	: m_filename(path), m_closed(false)
{
    static const char * const compatibleBrands[] = { "M4A ", "mp42" };
    void (MP4FileX::*create)(const char *, uint32_t, int, int,
	    char*, uint32_t, char **, uint32_t);
    if (temp) m_filename = L"qaac.int";
    try {
	create = temp ? &MP4FileX::CreateTemp : &MP4FileX::Create;
	(m_mp4file.*create)(
		    w2m(m_filename, utf8_codecvt_facet()).c_str(),
		    0, // flags
		    1, // add_ftypes
		    0, // add_iods
		    "M4A ", // majorBrand
		    0, // minorVersion
		    const_cast<char**>(compatibleBrands), 
		    array_size(compatibleBrands));
    } catch (mp4v2::impl::Exception *e) {
	handle_mp4error(e);
    }
}

void MP4SinkBase::close()
{
    if (!m_closed) {
	m_closed = true;
	try {
	    m_mp4file.Close();
	} catch (mp4v2::impl::Exception *e) {
	    handle_mp4error(e);
	}
    }
}

MP4Sink::MP4Sink(const std::wstring &path, EncoderBase &encoder, bool temp)
	: MP4SinkBase(path, temp)
{
    const AudioStreamBasicDescription &format
	= encoder.getOutputBasicDescription();
    uint32_t sample_rate = static_cast<uint32_t>(format.mSampleRate);
    uint32_t frame_length = format.mFramesPerPacket;
    if (format.mFormatID == 'aach') {
	sample_rate /= 2;
	frame_length /= 2;
    }
    try {
	m_mp4file.SetTimeScale(sample_rate);
	m_track_id = m_mp4file.AddAudioTrack(
			sample_rate, frame_length, MP4_MPEG4_AUDIO_TYPE);
	/*
	 * According to ISO 14496-12 8.16.3, 
	 * ChannelCount of AusioSampleEntry is either 1 or 2.
	 */
	m_mp4file.SetIntegerProperty(
		"moov.trak.mdia.minf.stbl.stsd.mp4a.channels",
		format.mChannelsPerFrame == 1 ? 1 : 2);

	uint8_t level;
	if (format.mChannelsPerFrame < 3)
	    level = sample_rate <= 24000 ? 0x28 : 0x29;
	else
	    level = sample_rate <= 48000 ? 0x2A : 0x2B;

	//m_mp4file.SetAudioProfileLevel(level);

	std::vector<char> cookie;
	encoder.getMagicCookie(&cookie);
	if (cookie.size() < 32 ||
		std::memcmp(&cookie[26], "\x05\x80\x80\x80", 4))
	    throw std::runtime_error("Unknown magic cookie format!!!");
	int config_size = static_cast<unsigned char>(cookie[30]);

	m_mp4file.SetTrackESConfiguration(
		m_track_id,
		reinterpret_cast<uint8_t*>(&cookie[31]),
		config_size);

    } catch (mp4v2::impl::Exception *e) {
	handle_mp4error(e);
    }
}

static void noop(void *) {}

ADTSSink::ADTSSink(const std::wstring &path, EncoderBase &encoder)
{
    if (path == L"-") {
#if defined(_MSC_VER) || defined(__MINGW32__)
	_setmode(_fileno(stdout), _O_BINARY);
#endif
	m_fp = file_ptr_t(stdout, noop);
    } else {
	m_fp = file_ptr_t(wfopenx(path.c_str(), L"wb"), fclose);
    }
    const AudioStreamBasicDescription &format
	= encoder.getOutputBasicDescription();
    uint32_t sample_rate = static_cast<uint32_t>(format.mSampleRate);
    if (format.mFormatID == 'aach')
	sample_rate /= 2;
    m_sample_rate_index = getSamplingRateIndex(sample_rate);
    AudioChannelLayoutX layout;
    encoder.getChannelLayout(&layout);
    m_channel_config = getChannelConfig(layout);
}

void ADTSSink::writeSamples(const void *data, size_t length, size_t nsamples)
{
    const unsigned profile = 1;
    unsigned char adts[] = "\xff\xf1\x00\x00\x00\x1f\xfc";
    size_t len = length + 7;

    adts[2] |= profile << 6;
    adts[2] |= m_sample_rate_index << 2;
    adts[2] |= (m_channel_config & 4) >> 2;
    adts[3] |= (m_channel_config & 3) << 6;
    adts[3] |= len >> 11;
    adts[4] |= (len >> 3) & 0xff;
    adts[5] |= (len & 7) << 5;
    std::fwrite(adts, 1, 7, m_fp.get());
    std::fwrite(data, 1, length, m_fp.get());
    if (ferror(m_fp.get()))
	throw std::runtime_error("write error");
}
