#include <algorithm>
#include "utf8_codecvt_facet.hpp"
#include "strcnv.h"
#include "alacsink.h"

static
bool get32BE(uint8_t const ** pos, const uint8_t *end, uint32_t *n)
{
    if (end - *pos < 4) return false;
    const uint8_t *p = *pos;
    *n = ((p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3]);
    *pos += 4;
    return true;
}

static
void parseMagicCookieALAC(const std::vector<uint8_t> &cookie,
	std::vector<uint8_t> *alac,
	std::vector<uint8_t> *chan)
{
    const uint8_t *beg = &cookie[0];
    const uint8_t *end = beg + cookie.size();
    uint32_t chunk_size, chunk_name;

    while (get32BE(&beg, end, &chunk_size)) {
	if (chunk_size < 8 || !get32BE(&beg, end, &chunk_name))
	    break;
	chunk_size -= 8;
	if (chunk_name == 'alac')
	    std::copy(beg + 4, beg + chunk_size, std::back_inserter(*alac));
	else if (chunk_name == 'chan')
	    std::copy(beg + 4, beg + chunk_size, std::back_inserter(*chan));
	beg += chunk_size;
    }
}

ALACSink::ALACSink(const std::wstring &path,
	const std::vector<uint8_t> &magicCookie, bool temp)
	: MP4SinkBase(path, temp)
{
    try {
	std::vector<uint8_t> alac, chan;
	parseMagicCookieALAC(magicCookie, &alac, &chan);
	if (alac.size() != 24)
	    throw std::runtime_error("Invalid ALACSpecificConfig!");
	if (chan.size() && chan.size() != 12)
	    throw std::runtime_error("Invalid ALACChannelLayout!");

	m_track_id = m_mp4file.AddAlacAudioTrack(
		&alac[0], chan.size() ? &chan[0] : 0);
    } catch (mp4v2::impl::Exception *e) {
	handle_mp4error(e);
    }
}
