#ifndef MMTISOBMFFSOURCE_H
#define MMTISOBMFFSOURCE_H

#include <mmtisobmff/reader/reader.h>
#include <mmtisobmff/reader/trackreader.h>
#include "ISource.h"
#include "PacketDecoder.h"
#include "IInputStream.h"
#include "util.h"
#include "MP4Edits.h"

class MMTISOBMFFSource: public ISeekableSource, public ITagParser, public IChapterParser
{
    std::unique_ptr<mmt::isobmff::CIsobmffReader> m_movieReader;
    std::unique_ptr<mmt::isobmff::CGenericAudioTrackReader> m_trackReader;
    mmt::isobmff::CMovieInfo m_movieInfo;
    mmt::isobmff::CTrackInfo m_trackInfo;
    ca::AudioStreamBasicDescription m_iasbd, m_oasbd;
    std::vector<uint32_t> m_chanmap;
    std::shared_ptr<IPacketDecoder> m_decoder;
    MP4Edits m_edits;
    std::vector<uint8_t> m_packetBuffer;
    std::vector<uint8_t> m_rawDecodeBuffer;
    util::FIFO<uint8_t>  m_decodeBuffer;
    int64_t  m_position;
    unsigned m_currentEdit;
    int64_t m_currentEditEndPosition;
    int64_t  m_nextPacket;
    std::map<std::string, std::string> m_tags;
    std::vector<misc::chapter_t> m_chapters;
public:
    MMTISOBMFFSource(std::shared_ptr<IInputStream> stream);
    uint64_t length() const
    {
        return m_edits.totalDuration();
    }
    const ca::AudioStreamBasicDescription &getSampleFormat() const
    {
        return m_oasbd;
    }
    const std::vector<uint32_t> *getChannels() const
    {
        return m_chanmap.size() ? &m_chanmap: 0;
    }
    int64_t getPosition() { return m_position; }
    size_t readSamples(void *buffer, size_t nsamples);
    void seekTo(int64_t count);
    const std::map<std::string, std::string> &getTags() const { return m_tags; }
    const std::vector<misc::chapter_t> &getChapters() const { return m_chapters; }
private:
    bool readPacket(std::vector<uint8_t>* buffer);
    int64_t mediaTimeToDecodeTime(int64_t mediaTime);
    int64_t decodeTimeToMediaTime(int64_t decodeTime);
    void fillDecodeBuffer();
    void setupALAC();
    void setupFLAC();
    void setupMPEG4Audio();
    void setupOpus();
    unsigned getMaxFrameDependency();
    void getQTChapters();
};

#endif
