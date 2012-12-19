#include <algorithm>
#include "utf8_codecvt_facet.hpp"
#include "strcnv.h"
#include "alacsink.h"
#include "ioabst.h"

static
void parseMagicCookieALAC(const std::vector<uint8_t> &cookie,
        std::vector<uint8_t> *alac,
        std::vector<uint8_t> *chan)
{
    std::string s(cookie.begin(), cookie.end());
    if (s.find("frmaalac") == std::string::npos) {
        static const char *const hd = "\x00\x00\x00\x0c" "frmaalac"
            "\x00\x00\x00\x24" "alac" "\x00\x00\x00\x00";
        s = std::string(hd, hd + 24) + s;
    }
    MemoryReader reader(s.c_str(), s.size());
    uint32_t chunk_size, chunk_name;

    while (reader.read32be(&chunk_size)) {
        if (chunk_size < 8 || !reader.read32be(&chunk_name))
            break;
        chunk_size -= 8;
        if (chunk_name == 'alac') {
            chunk_size -= 4;
            if (reader.skip(4) != 4) break;
            alac->resize(chunk_size);
            if (reader.read(&(*alac)[0], chunk_size) != chunk_size) break;
        } else if (chunk_name == 'chan') {
            chunk_size -= 4;
            if (reader.skip(4) != 4) break;
            chan->resize(chunk_size);
            if (reader.read(&(*chan)[0], chunk_size) != chunk_size) break;
        } else if (reader.skip(chunk_size) != chunk_size)
            break;
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
