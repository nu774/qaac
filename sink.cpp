#include <algorithm>
#include "utf8_codecvt_facet.hpp"
#include "strcnv.h"
#include "sink.h"
#if defined(_MSC_VER) || defined(__MINGW32__)
#include <io.h>
#include <fcntl.h>
#endif

static
bool getDescripterHeader(const uint8_t **p, const uint8_t *end,
			 int *tag, uint32_t *size)
{
    *size = 0;
    if (*p < end) {
	*tag = *(*p)++;
	while (*p < end) {
	    int n = *(*p)++;
	    *size = (*size << 7) | (n & 0x7f);
	    if (!(n & 0x80)) return true;
	}
    }
    return false;
}

static
void parseMagicCookieAAC(const std::vector<char> &cookie,
			 std::vector<uint8_t> *decSpecificConfig)
{
    /*
     * QT's "Magic Cookie" for AAC is just a esds descripter.
     * We obtain only decSpecificConfig from it, and discard others.
     */
    const uint8_t *p = reinterpret_cast<const uint8_t*>(&cookie[0]);
    const uint8_t *end = p + cookie.size();
    int tag;
    uint32_t size;
    while (getDescripterHeader(&p, end, &tag, &size)) {
	switch (tag) {
	case 3: // esds
	    /*
	     * ES_ID: 16
	     * streamDependenceFlag: 1
	     * URLFlag: 1
	     * OCRstreamFlag: 1
	     * streamPriority: 5
	     *
	     * (flags are all zero, so other atttributes are not present)
	     */
	    p += 3;
	    break;
	case 4: // decConfig
	    /*
	     * objectTypeId: 8
	     * streamType: 6
	     * upStream: 1
	     * reserved: 1
	     * bufferSizeDB: 24
	     * maxBitrate: 32
	     * avgBitrate: 32
	     *
	     * QT gives constant value for bufferSizeDB, max/avgBitrate
	     * depending on encoder settings.
	     * On the other hand, mp4v2 calculates and sets them
	     * using "real" values, when finished media writing;
	     * Therefore, these values will be different from QT.
	     */
	    p += 13;
	    break;
	case 5: // decSpecificConfig
	    {
		std::vector<uint8_t> vec(size);
		std::memcpy(&vec[0], p, size);
		decSpecificConfig->swap(vec);
	    }
	    return;
	default:
	    p += size;
	}
    }
    throw std::runtime_error(
	    "Magic cookie format is different from expected!!");
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
	m_mp4file.ResetFile();
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

MP4Sink::MP4Sink(const std::wstring &path, IEncoderOutputInfo *info, bool temp)
	: MP4SinkBase(path, temp)
{
    AudioStreamBasicDescription format;
    info->getBasicDescription(&format);
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
	info->getMagicCookie(&cookie);
	std::vector<uint8_t> decSpecificConfig;
	parseMagicCookieAAC(cookie, &decSpecificConfig);

	m_mp4file.SetTrackESConfiguration(
		m_track_id,
		&decSpecificConfig[0],
		decSpecificConfig.size());

    } catch (mp4v2::impl::Exception *e) {
	handle_mp4error(e);
    }
}

static void noop(void *) {}

ADTSSink::ADTSSink(const std::wstring &path, IEncoderOutputInfo *info)
{
    if (path == L"-") {
#if defined(_MSC_VER) || defined(__MINGW32__)
	_setmode(_fileno(stdout), _O_BINARY);
#endif
	m_fp = file_ptr_t(stdout, noop);
    } else {
	m_fp = file_ptr_t(wfopenx(path.c_str(), L"wb"), fclose);
    }
    std::vector<char> cookie;
    info->getMagicCookie(&cookie);
    std::vector<uint8_t> decSpecificConfig;
    parseMagicCookieAAC(cookie, &decSpecificConfig);
    unsigned objtype = decSpecificConfig[0] >> 3;
    m_sample_rate_index
	= ((decSpecificConfig[0] & 3)<<1) | (decSpecificConfig[1] >> 7);
    unsigned offset = m_sample_rate_index == 0xf ? 3 : 0;
    m_channel_config = (decSpecificConfig[1 + offset] >> 3) & 0xf;
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
