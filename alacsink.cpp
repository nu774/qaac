#include <algorithm>
#include "utf8_codecvt_facet.hpp"
#include "strcnv.h"
#include "alacsink.h"

static
bool get32BE(char const ** pos, const char *end, uint32_t *n)
{
    if (end - *pos < 4) return false;
    const unsigned char *p = reinterpret_cast<const unsigned char *>(*pos);
    *n = ((p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3]);
    *pos += 4;
    return true;
}

static
void parseMagicCookieALAC(const std::vector<char> &cookie,
	std::vector<char> *alac,
	std::vector<char> *chan)
{
    const char *beg = &cookie[0];
    const char *end = beg + cookie.size();
    uint32_t chunk_size, chunk_name;

    while (get32BE(&beg, end, &chunk_size)) {
	if (chunk_size < 8 || !get32BE(&beg, end, &chunk_name))
	    break;
	chunk_size -= 8;
	if (chunk_name == 'alac')
	    std::copy(beg, beg + chunk_size, std::back_inserter(*alac));
	else if (chunk_name == 'chan')
	    std::copy(beg, beg + chunk_size, std::back_inserter(*chan));
	beg += chunk_size;
    }
}

ALACSink::ALACSink(const std::wstring &path, EncoderBase &encoder, bool temp)
	: MP4SinkBase(path, temp)
{
    AudioStreamBasicDescription format;
    encoder.getBasicDescription(&format);
    uint32_t sample_rate = static_cast<uint32_t>(format.mSampleRate);
    try {
	m_mp4file.SetTimeScale(sample_rate);
	std::vector<char> cookie, alac, chan;
	encoder.getMagicCookie(&cookie);
	parseMagicCookieALAC(cookie, &alac, &chan);
	/* XXX
	   OutputBasicDescription.mBitsPerChannel is always zero for ALAC.
	   Therefore, we use InputBasicDescription instead.
         */
	AudioStreamBasicDescription idesc;
	encoder.getInputBasicDescription(&idesc);
	uint32_t bps = idesc.mBitsPerChannel;
	m_track_id = m_mp4file.AddAlacAudioTrack(sample_rate,
		bps > 16 ? 24 : 16,
		reinterpret_cast<uint8_t*>(&alac[0]),
		alac.size());
    } catch (mp4v2::impl::Exception *e) {
	handle_mp4error(e);
    }
}
