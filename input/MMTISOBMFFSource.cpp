#include <cstdio>
#include <cstring>
#include <mmtisobmff/reader/input.h>
#include "MMTISOBMFFSource.h"
#include "IInputStream.h"
#include "ilo/memory.h"
#include "ilo/string_utils.h"
#include "mmtisobmff/configdescriptor/mp4a_decoderconfigrecord.h"
#include "mmtisobmff/reader/reader.h"
#include "mmtisobmff/reader/trackreader.h"
#include "mmtisobmff/types.h"
#include "util.h"
#include "ascutil.h"
#include "cautil.h"
#include "chanmap.h"
#ifdef QAAC
#include "CoreAudioPacketDecoder.h"
#include "MPAHeader.h"
#else
#include "ALACPacketDecoder.h"
#endif
#include "FLACPacketDecoder.h"
#include "OpusPacketDecoder.h"
#include "metadata.h"

class InputStreamInput: public mmt::isobmff::IIsobmffInput {
    std::shared_ptr<IInputStream> m_stream;
public:
    InputStreamInput(std::shared_ptr<IInputStream> stream)
    : m_stream(stream)
    {}
    size_t read(ilo::ByteBuffer::iterator inBegin, ilo::ByteBuffer::iterator inEnd)
    {
        int n = m_stream->read(&*inBegin, inEnd - inBegin);
        return (std::max)(0, n);
    }
    void seek(mmt::isobmff::pos_type pos)
    {
        int64_t off = m_stream->seek(pos, SEEK_SET);
        if (off < 0) {
            throw std::runtime_error("seek() failed");
        }
    }
    void seek(mmt::isobmff::offset_type offset, mmt::isobmff::SeekingOrigin origin)
    {
        int whence = [origin]() {
            switch (origin) {
            case mmt::isobmff::SeekingOrigin::end:
                return SEEK_END;
            case mmt::isobmff::SeekingOrigin::cur:
                return SEEK_CUR;
            }
            return SEEK_SET;
        }();
        int64_t off = m_stream->seek(offset, whence);
        if (off < 0) {
            throw std::runtime_error("seek() failed");
        }
    }
    mmt::isobmff::pos_type tell()
    {
        int64_t off = m_stream->tell();
        if (off < 0) {
            throw std::runtime_error("tell() failed");
        }
        return off;
    }
    bool isEOI()
    {
        char c;
        int n = m_stream->read(&c, 1);
        if (n == 1) {
            m_stream->seek(-1, SEEK_CUR);
        }
        return n == 0;
    }
    std::unique_ptr<mmt::isobmff::IIsobmffInput> clone()
    {
        return ilo::make_unique<InputStreamInput>(m_stream);
    }
};

MMTISOBMFFSource::MMTISOBMFFSource(std::shared_ptr<IInputStream> stream)
    : m_nextPacket(0)
    , m_position(0)
{
    memset(&m_iasbd, 0, sizeof(m_iasbd));
    memset(&m_oasbd, 0, sizeof(m_oasbd));
    {
        util::FilePositionSaver _(stream);
        stream->seek(0, SEEK_SET);
        char buf[8];
        if (stream->read(buf, 8) != 8 || std::memcmp(&buf[4], "ftyp", 4))
            throw std::runtime_error("Not an MP4 file");
    }
    auto input = ilo::make_unique<InputStreamInput>(stream);
    m_movieReader = ilo::make_unique<mmt::isobmff::CIsobmffReader>(std::move(input));
    m_movieInfo = m_movieReader->movieInfo();
    auto trackInfos = m_movieReader->trackInfos();
    auto it = std::find_if(std::begin(trackInfos), std::end(trackInfos), [](const mmt::isobmff::CTrackInfo &track) {
        return track.type == mmt::isobmff::TrackType::audio;
    });
    if (it == std::end(trackInfos)) {
        throw std::runtime_error("audio track not found");
    }
    int index = it - std::begin(trackInfos);
    m_trackInfo = trackInfos[index];
    m_trackReader = m_movieReader->trackByIndex<mmt::isobmff::CGenericAudioTrackReader>(index);
    auto codingName = m_trackReader->codingName();
    if (codingName == ilo::toFcc("fLaC")) {
        setupFLAC();
    } else if (codingName == ilo::toFcc("Opus")) {
        setupOpus();
    } else if (codingName == ilo::toFcc("alac")) {
        setupALAC();
#ifdef QAAC
    } else if (codingName == ilo::toFcc("mp4a")) {
        setupMPEG4Audio();
#endif
    } else {
        throw std::runtime_error("unsupported codec");
    }
    m_decodeBuffer.set_unit(m_oasbd.mBytesPerFrame);

    if (!m_movieInfo.userData.empty()) {
        for (auto&& userData : m_movieInfo.userData) {
            auto items = M4A::parseUdtaMeta(userData.data(), userData.size());
            auto tags = M4A::convertToStringTags(items);
            m_tags.insert(tags.begin(), tags.end());
		}
    }
    bool inaccurate_edit = m_movieInfo.timeScale < m_trackInfo.timescale;
    if (!m_trackInfo.editList.empty()) {
        if (inaccurate_edit && m_trackInfo.editList.size() == 1) {
            if (m_tags.find("iTunSMPB") == m_tags.end()) {
                int64_t off = m_trackInfo.editList[0].mediaTime;
                m_edits.addEntry(off, m_trackInfo.duration - off);
            }
        } else {
            for (auto&& edit : m_trackInfo.editList) {
                m_edits.addEntry(edit.mediaTime, static_cast<double>(edit.segmentDuration) / m_movieInfo.timeScale * m_trackInfo.timescale + .5);
            }
        }
    }
    if (m_edits.count() == 0) {
        auto it = m_tags.find("iTunSMPB");
        if (it != m_tags.end()) {
            unsigned dummy, priming, padding;
            uint64_t duration;
            if (std::sscanf(it->second.c_str(), "%x %x %x %llx", &dummy, &priming, &padding, &duration) == 4) {
                m_edits.addEntry(priming, duration);
            }
        }
    }
    getQTChapters();
    if (m_chapters.empty() && !m_movieInfo.userData.empty()) {
		for (auto&& userData : m_movieInfo.userData) {
            auto chapters = M4A::parseUdtaChpl(userData.data(), userData.size());
            if (chapters.empty()) continue;
            double firstChapterTime = chapters[0].second;
            if (m_edits.count() == 0) {
                m_edits.addEntry(firstChapterTime * m_oasbd.mSampleRate, m_trackInfo.duration - firstChapterTime * m_trackInfo.timescale);
            }
            for (auto&& chapter : chapters) {
                chapter.second -= firstChapterTime;
            }
            chapters.emplace_back("End", length() / m_trackInfo.timescale);
            for (size_t i = 0; i < chapters.size() - 1; ++i) {
                std::string title = chapters[i].first;
                double duration = chapters[i + 1].second - chapters[i].second;
                m_chapters.emplace_back(title, duration);
            }
        }
    }
    if (m_edits.count() == 0) {
        m_edits.addEntry(0, m_trackInfo.duration);
    }
    if (m_oasbd.mSampleRate != m_trackInfo.timescale) {
        m_edits.scaleShift(m_oasbd.mSampleRate / m_trackInfo.timescale);
    }
    seekTo(0);
}

size_t MMTISOBMFFSource::readSamples(void *buffer, size_t nsamples)
{
    if (m_decodeBuffer.count() == 0) {
        fillDecodeBuffer();
    }
    if (nsamples > m_decodeBuffer.count())
        nsamples = m_decodeBuffer.count();
    if (nsamples > 0) {
        std::memcpy(buffer, m_decodeBuffer.read(nsamples), nsamples * m_oasbd.mBytesPerFrame);
        m_position += nsamples;
    }
    return nsamples;
}

void MMTISOBMFFSource::seekTo(int64_t count)
{
    if (count >= length()) {
        m_nextPacket = m_trackInfo.sampleCount;
        return;
    }
    m_decodeBuffer.reset();
    m_decoder->reset();
    int64_t offsetInEdit;
    m_currentEdit = m_edits.editForPosition(count, &offsetInEdit);
    int64_t offsetInMediaTime = m_edits.mediaOffset(m_currentEdit) + offsetInEdit;
    m_position = count;
    m_nextPacket = std::max<int64_t>((offsetInMediaTime - getMaxFrameDependency() * m_iasbd.mFramesPerPacket) / m_iasbd.mFramesPerPacket, 0LL);
    int prerollSamples = offsetInMediaTime - m_nextPacket * m_iasbd.mFramesPerPacket;
    while (prerollSamples > 0) {
        readPacket(&m_packetBuffer);
        size_t samples = m_decoder->decode(m_packetBuffer, &m_rawDecodeBuffer);
        if (samples > prerollSamples) {
            m_decodeBuffer.reserve(samples - prerollSamples);
            memcpy(m_decodeBuffer.write_ptr(), &m_rawDecodeBuffer[prerollSamples * m_oasbd.mBytesPerFrame], (samples - prerollSamples) * m_oasbd.mBytesPerFrame);
            m_decodeBuffer.commit(samples - prerollSamples);
        }
        prerollSamples -= samples;
    }
    m_currentEditEndPosition = m_edits.endPosition(m_currentEdit);
}

bool MMTISOBMFFSource::readPacket(std::vector<uint8_t> *buffer)
{
    mmt::isobmff::CSample sample;
    m_trackReader->sampleByIndex(m_nextPacket, sample, true);
    if (sample.empty()) {
        buffer->resize(0);
        return false;
    }
    buffer->swap(sample.rawData);
    ++m_nextPacket;
    return true;
}

int64_t MMTISOBMFFSource::mediaTimeToDecodeTime(int64_t mediaTime)
{
    return static_cast<double>(mediaTime) / m_trackInfo.timescale * m_oasbd.mSampleRate + .5;
}

int64_t MMTISOBMFFSource::decodeTimeToMediaTime(int64_t decodeTime)
{
    return static_cast<double>(decodeTime) / m_oasbd.mSampleRate * m_trackInfo.timescale + .5;
}

void MMTISOBMFFSource::fillDecodeBuffer()
{
    while (m_decodeBuffer.count() == 0) {
        if (m_position + m_decodeBuffer.count() >= m_currentEditEndPosition)
            seekTo(m_position);
        bool ok = readPacket(&m_packetBuffer);
        int nsamples = m_decoder->decode(m_packetBuffer, &m_rawDecodeBuffer);
        if (m_position + m_decodeBuffer.count() + nsamples > m_currentEditEndPosition) {
            nsamples = std::max<int64_t>(0LL, m_currentEditEndPosition - m_position - int(m_decodeBuffer.count()));
        }
        if (!ok && nsamples == 0) break;
        if (nsamples > 0) {
            m_decodeBuffer.reserve(nsamples);
            std::memcpy(m_decodeBuffer.write_ptr(), m_rawDecodeBuffer.data(), m_rawDecodeBuffer.size());
            m_decodeBuffer.commit(nsamples);
        }
    }
}

void MMTISOBMFFSource::setupALAC()
{
    auto alac = m_trackReader->decoderConfigRecord();
    if (alac.size() < 24)
        throw std::runtime_error("Malformed ALAC magic cookie");
    {
        uint32_t framelen, timescale;
        std::memcpy(&framelen,  &alac[0], 4);
        framelen  = util::b2host32(framelen);
        std::memcpy(&timescale, &alac[20], 4);
        timescale = util::b2host32(timescale);
        uint8_t  nchannels = alac[9];
        uint8_t  bits      = alac[5];
        memset(&m_iasbd, 0, sizeof m_iasbd);
        m_iasbd.mSampleRate = timescale;
        m_iasbd.mFormatID   = 'alac';
        switch (bits) {
        case 16: m_iasbd.mFormatFlags = 1; break;
        case 20: m_iasbd.mFormatFlags = 2; break;
        case 24: m_iasbd.mFormatFlags = 3; break;
        case 32: m_iasbd.mFormatFlags = 4; break;
        }
        m_iasbd.mFramesPerPacket  = framelen;
        m_iasbd.mChannelsPerFrame = nchannels;
    }
    if (m_iasbd.mChannelsPerFrame <= 8) {
        AudioChannelLayout acl = { 0 };
        acl.mChannelLayoutTag = chanmap::getALACChannelLayoutTag(m_iasbd.mChannelsPerFrame);
        m_chanmap = chanmap::getChannels(&acl);
	}
#ifdef QAAC
    m_decoder = std::make_shared<CoreAudioPacketDecoder>(cautil::toNative(m_iasbd));
#else
    m_decoder = std::make_shared<ALACPacketDecoder>(m_iasbd);
#endif
    m_decoder->setMagicCookie(alac);
    m_oasbd = m_decoder->getSampleFormat();
}

void MMTISOBMFFSource::setupFLAC()
{
    auto dfLa = m_trackReader->decoderConfigRecord();
    dfLa.erase(std::begin(dfLa), std::begin(dfLa) + 4);
    m_decoder = std::make_shared<FLACPacketDecoder>();
    m_decoder->setMagicCookie(dfLa);
    m_iasbd = std::dynamic_pointer_cast<FLACPacketDecoder>(m_decoder)->getInputFormat();
    m_oasbd = m_decoder->getSampleFormat();
}

#ifdef QAAC
void MMTISOBMFFSource::setupMPEG4Audio()
{
    auto esds = m_trackReader->decoderConfigRecord();
    ilo::ByteBuffer::const_iterator esdsBegin = std::begin(esds);
    mmt::isobmff::config::CMp4aDecoderConfigRecord configRec(esdsBegin, std::end(esds));
    auto object_type = configRec.objectTypeIndication();
    /*
     * 0x40: MPEG-4
     * 0x67: MPEG-2 AAC
     * 0x69: MPEG-2 layer 1/2/3
     * 0x6b: MPEG-1 layer 1/2/3
     */
    switch (object_type) {
    case 0x40:
    case 0x67:
    {
        auto asc = ascutil::parseMagicCookieAAC(esds);
        ascutil::parseASC(asc, &m_iasbd, &m_chanmap);
        if (m_iasbd.mFormatID != 'aac ' &&
            m_iasbd.mFormatID != 'aach' &&
            m_iasbd.mFormatID != 'aacp')
            throw std::runtime_error("Not supported input codec");
        m_decoder = std::make_shared<CoreAudioPacketDecoder>(cautil::toNative(m_iasbd));
        m_decoder->setMagicCookie(esds);
        m_oasbd = m_decoder->getSampleFormat();
        break;
    }
    case 0x69:
    case 0x6b:
    {
        readPacket(&m_packetBuffer);
        m_nextPacket = 0;
        MPAHeader header(m_packetBuffer.data());
        uint32_t layer_tab[] = { 0, '.mp3', '.mp2', '.mp1' };
        memset(&m_iasbd, 0, sizeof m_iasbd);
        m_iasbd.mSampleRate       = header.sample_rate();
        m_iasbd.mFormatID         = layer_tab[header.layer];
        m_iasbd.mFramesPerPacket  = header.samples_per_frame();
        m_iasbd.mChannelsPerFrame = header.is_mono() ? 1 : 2; 
        m_decoder = std::make_shared<CoreAudioPacketDecoder>(cautil::toNative(m_iasbd));
        m_oasbd = m_decoder->getSampleFormat();
        break;
    }
    default:
        throw std::runtime_error("Not supported input codec");
    }
}
#endif

void MMTISOBMFFSource::setupOpus()
{
    auto dOps = m_trackReader->decoderConfigRecord();
    m_decoder = std::make_shared<OpusPacketDecoder>();
    m_decoder->setMagicCookie(dOps);
    readPacket(&m_packetBuffer);
    m_decoder->decode(m_packetBuffer, &m_rawDecodeBuffer);
    m_nextPacket = 0;
    m_iasbd = std::dynamic_pointer_cast<OpusPacketDecoder>(m_decoder)->getInputFormat();
    m_oasbd = m_decoder->getSampleFormat();
}

unsigned MMTISOBMFFSource::getMaxFrameDependency()
{
    switch (m_iasbd.mFormatID) {
    case 'alac':
    case 'fLaC':
        return 0;
    case '.mp3': return 10;
    case 'aach':
    case 'aacp':
        return m_iasbd.mSampleRate / 2 / m_iasbd.mFramesPerPacket;
    case 'opus':
        return 4;
    }
    return 1;
}

void MMTISOBMFFSource::getQTChapters()
{
    auto trackInfos = m_movieReader->trackInfos();
    std::vector<uint32_t>* chapter_tracks = nullptr;
    for (auto&& trackInfo : trackInfos) {
        auto it = trackInfo.references.find(ilo::toFcc("chap"));
        if (it != trackInfo.references.end()) {
            chapter_tracks = &it->second;
            break;
        }
    }
    if (chapter_tracks == nullptr)
        return;
    auto itt = std::find_if(std::begin(trackInfos), std::end(trackInfos), [chapter_tracks](const mmt::isobmff::CTrackInfo& track) {
        return track.type == mmt::isobmff::TrackType::text
            && !track.enabled
            && std::find(std::begin(*chapter_tracks), std::end(*chapter_tracks), track.trackId) != std::end(*chapter_tracks);
    });
    if (itt == std::end(trackInfos))
        return;
    auto tindex = itt - std::begin(trackInfos);
    auto ttrackInfo = trackInfos[tindex];
    auto trackReader = m_movieReader->trackByIndex<mmt::isobmff::CGenericTrackReader>(tindex);
    mmt::isobmff::CSample sample;
    while (true) {
        auto extraInfo = trackReader->nextSample(sample);
        if (sample.rawData.size() < 2)
            break;
        auto len = (sample.rawData[0] << 8 | sample.rawData[1]);
        if (sample.rawData.size() < 2 + len)
            continue;
        std::string text(sample.rawData.data() + 2, sample.rawData.data() + 2 + len);
        m_chapters.emplace_back(std::make_pair(text, static_cast<double>(sample.duration) / ttrackInfo.timescale));
    }
}
