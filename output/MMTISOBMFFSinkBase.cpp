#include "MMTISOBMFFSinkBase.h"
#include <cstring>
#include "strutil.h"
#include "mmtisobmff/types.h"
#include "metadata.h"

void MMTISOBMFFSinkBase::writeSamples(const void *data, size_t length, size_t nsamples)
{
    mmt::isobmff::CSample sample(length);
    sample.rawData.resize(length);
    std::memcpy(sample.rawData.data(), data, length);
    sample.duration = nsamples;
    if (m_gaplessMode & MODE_EDTS) {
        sample.sampleGroupInfo =
            mmt::isobmff::SSampleGroupInfo(mmt::isobmff::SampleGroupType::roll, -1, 0);
    }
    m_trackWriter->addSample(sample);
    m_totalDuration += nsamples;
}

void MMTISOBMFFSinkBase::finishWrite(const ca::AudioFilePacketTableInfo &info)
{
    if ((m_gaplessMode & MODE_EDTS) &&
        (info.mPrimingFrames || info.mNumberValidFrames)) {
        mmt::isobmff::SEdit edit;
        edit.mediaTime = info.mPrimingFrames;
        edit.segmentDuration = info.mNumberValidFrames;
        m_trackWriter->addEditListEntry(edit);
    }
    if ((m_gaplessMode & MODE_ITUNSMPB) &&
        (info.mPrimingFrames || info.mNumberValidFrames)) {
        std::string value = strutil::format(iTunSMPB_template,
            info.mPrimingFrames,
            uint32_t(m_totalDuration - info.mPrimingFrames
                     - info.mNumberValidFrames),
            uint32_t(info.mNumberValidFrames >> 32),
            uint32_t(info.mNumberValidFrames & 0xffffffff));
        m_tags["iTunSMPB"] = value;
    }
    if (!m_chapters.empty()) {
        mmt::isobmff::STextTrackConfig trackConfig;
        trackConfig.refTrackId = 1;
        trackConfig.mediaTimescale = m_mediaTimescale;
        auto textTrackWriter = m_movieWriter->trackWriter<mmt::isobmff::CTextTrackWriter>(trackConfig);
        for (auto& chapter : m_chapters) {
            auto &title = chapter.first;
            auto duration = static_cast<uint64_t>(chapter.second * trackConfig.mediaTimescale + .5);
            textTrackWriter->addSample(title, duration);
        }
    }
    std::vector<M4A::ITMFItem> items = M4A::convertToM4aTags(m_tags);
    for (auto &&artwork: m_artworks) {
        M4A::ITMFItem item;
        item.code = Tag::kArtwork;
        item.type = M4A::getImageFileType(artwork.data(), artwork.size());
        item.value.assign(artwork.data(), artwork.size());
        items.push_back(item);
    }
    std::vector<uint8_t> meta = M4A::serializeUdtaMeta(items);
    std::vector<std::vector<uint8_t>> userData = { meta };
    if (!m_chapters.empty()) {
        std::vector<misc::chapter_t> chapters;
        double chapterTime = static_cast<double>(info.mPrimingFrames) / m_mediaTimescale;
        for (auto& chapter : m_chapters) {
            chapters.emplace_back(chapter.first, chapterTime);
            chapterTime += chapter.second;
        }
        std::vector<uint8_t> chpl = M4A::serializeUdtaChpl(chapters);
        userData.push_back(chpl);
    }
    m_movieWriter->setUserData(userData);
    m_movieWriter->close();
}
