#include "MMTISOBMFFALACSink.h"
#include <cstring>
#include "util.h"

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

MMTISOBMFFALACSink::MMTISOBMFFALACSink(const std::string &path, const std::vector<uint8_t> &cookie) {
    std::vector<uint8_t> alac;
    std::vector<uint8_t> chan;
    parseMagicCookieALAC(cookie, &alac, &chan);
    uint32_t nchannels = alac[9];
    uint32_t timeScale;
    std::memcpy(&timeScale, alac.data() + 20, 4);
    timeScale = util::b2host32(timeScale);

    mmt::isobmff::CIsobmffFileWriter::SOutputConfig outputConfig;
    outputConfig.outputUri = path;
    m_movieConfig.majorBrand = ilo::toFcc("M4A ");
    m_movieConfig.compatibleBrands = { ilo::toFcc("mp42"), ilo::toFcc("isom") };
    m_movieConfig.movieTimeScale = timeScale;

    mmt::isobmff::SAlacTrackConfig trackConfig;
    trackConfig.mediaTimescale = timeScale;
    trackConfig.channelCount = nchannels;
    trackConfig.sampleRate = timeScale;
    trackConfig.configRecord = alac;
    m_mediaTimescale = trackConfig.mediaTimescale;

    m_movieWriter = ilo::make_unique<mmt::isobmff::CIsobmffFileWriter>(outputConfig, m_movieConfig);
    m_trackWriter = m_movieWriter->trackWriter<mmt::isobmff::CAlacTrackWriter>(trackConfig);
}
