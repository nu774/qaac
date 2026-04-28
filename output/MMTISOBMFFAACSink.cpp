#include "MMTISOBMFFAACSink.h"
#include "ascutil.h"

MMTISOBMFFAACSink::MMTISOBMFFAACSink(const std::string &path, const std::vector<uint8_t> &esds,
                                     uint32_t gapless_mode) {
    switch (gapless_mode) {
    case 0:  m_gaplessMode = MODE_ITUNSMPB; break;
    case 1:  m_gaplessMode = MODE_EDTS;     break;
    default: m_gaplessMode = MODE_BOTH;     break;
    }
    auto asc = ascutil::parseMagicCookieAAC(esds);
    ca::AudioStreamBasicDescription asbd;
    std::vector<uint32_t> channels;
    ascutil::parseASC(asc, &asbd, &channels);

    mmt::isobmff::CIsobmffFileWriter::SOutputConfig outputConfig;
    outputConfig.outputUri = path;
    m_movieConfig.majorBrand = ilo::toFcc("M4A ");
    m_movieConfig.compatibleBrands = { ilo::toFcc("mp42"), ilo::toFcc("isom") };
    m_movieConfig.movieTimeScale = asbd.mSampleRate;

    mmt::isobmff::SMp4aTrackConfig trackConfig;
    trackConfig.mediaTimescale = asbd.mSampleRate;
    trackConfig.channelCount = asbd.mChannelsPerFrame;
    trackConfig.sampleRate = asbd.mSampleRate;
    auto esds_begin = esds.begin();
    trackConfig.configRecord = ilo::make_unique<mmt::isobmff::config::CMp4aDecoderConfigRecord>(esds_begin, esds.end());
    m_mediaTimescale = trackConfig.mediaTimescale;

    m_movieWriter = ilo::make_unique<mmt::isobmff::CIsobmffFileWriter>(outputConfig, m_movieConfig);
    m_trackWriter = m_movieWriter->trackWriter<mmt::isobmff::CMp4aTrackWriter>(trackConfig);
}
