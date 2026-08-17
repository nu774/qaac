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
    uint32_t mediaRate = asbd.mSampleRate;
    if (asbd.mFormatID != 'aac ') {
        mediaRate /= 2;
        m_sampleDurationDivisor = 2;
    }

    mmt::isobmff::CIsobmffFileWriter::SOutputConfig outputConfig;
    outputConfig.outputUri = path;
    m_movieConfig.majorBrand = ilo::toFcc("M4A ");
    m_movieConfig.compatibleBrands = { ilo::toFcc("mp42"), ilo::toFcc("isom") };
    m_movieConfig.movieTimeScale = mediaRate;

    mmt::isobmff::SMp4aTrackConfig trackConfig;
    trackConfig.mediaTimescale = mediaRate;
    trackConfig.channelCount = asbd.mChannelsPerFrame;
    trackConfig.sampleRate = asbd.mSampleRate;
    auto esds_begin = esds.begin();
    trackConfig.configRecord = ilo::make_unique<mmt::isobmff::config::CMp4aDecoderConfigRecord>(esds_begin, esds.end());
    m_mediaTimescale = trackConfig.mediaTimescale;

    m_movieWriter = ilo::make_unique<mmt::isobmff::CIsobmffFileWriter>(outputConfig, m_movieConfig);
    m_trackWriter = m_movieWriter->trackWriter<mmt::isobmff::CMp4aTrackWriter>(trackConfig);
}

void MMTISOBMFFAACSink::writeSamples(const void *data, size_t length, size_t nsamples)
{
    MMTISOBMFFSinkBase::writeSamples(data, length, nsamples);

    uint32_t duration = static_cast<uint32_t>(nsamples / m_sampleDurationDivisor);
    m_recentSamples.push_back({ static_cast<uint32_t>(length), duration });
    m_recentSize += length;
    m_recentDuration += duration;
    updateMaxBitrate(false);
}

void MMTISOBMFFAACSink::updateMaxBitrate(bool finalize)
{
    while (!m_recentSamples.empty() &&
           m_recentDuration - m_recentSamples.front().duration >= m_mediaTimescale) {
        m_recentSize -= m_recentSamples.front().size;
        m_recentDuration -= m_recentSamples.front().duration;
        m_recentSamples.pop_front();
    }
    if (!m_recentDuration || (!finalize && m_recentDuration < m_mediaTimescale))
        return;
    uint32_t bitrate = static_cast<uint32_t>(
        m_recentSize * 8.0 * m_mediaTimescale / m_recentDuration + .5);
    if (bitrate > m_maxBitrate)
        m_maxBitrate = bitrate;
}

void MMTISOBMFFAACSink::writeBitrates(int avgBitrate)
{
    updateMaxBitrate(true);
    auto *trackWriter = static_cast<mmt::isobmff::CMp4aTrackWriter *>(m_trackWriter.get());
    trackWriter->updateBitrates(m_maxBitrate, static_cast<uint32_t>(avgBitrate));
}
