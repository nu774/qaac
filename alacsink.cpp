#include <algorithm>
#include "strutil.h"
#include "alacsink.h"

static
void parseMagicCookieALAC(const std::vector<uint8_t> &cookie,
	std::vector<uint8_t> *alac,
	std::vector<uint8_t> *chan)
{
    const uint8_t *cp = &cookie[0];
    const uint8_t *endp = cp + cookie.size();
    if (std::memcmp(cp + 4, "frmaalac", 8) == 0)
	cp += 24;
    if (endp - cp >= 24) {
	alac->resize(24);
	std::memcpy(&(*alac)[0], cp, 24);
	cp += 24;
	if (endp - cp >= 24 && !std::memcmp(cp + 4, "chan", 4)) {
	    chan->resize(12);
	    std::memcpy(&(*chan)[0], cp + 12, 12);
	}
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
