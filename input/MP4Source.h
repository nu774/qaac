#include "ISource.h"
#include "mp4v2wrapper.h"
#include "PacketDecoder.h"
#include "IInputStream.h"
#include "MP4Edits.h"

class MP4Source: public ISeekableSource, public ITagParser,
    public IChapterParser
{
    uint32_t m_track_id;
    int64_t  m_position;
    int64_t  m_nextPacket;
    std::shared_ptr<IPacketDecoder>    m_decoder;
    std::map<std::string, std::string> m_tags;
    std::vector<misc::chapter_t>     m_chapters;
    std::vector<uint32_t> m_chanmap;
    std::shared_ptr<IInputStream> m_stream;
    MP4FileX m_file;
    MP4Edits m_edits;
    std::vector<uint8_t> m_packetBuffer;
    std::vector<uint8_t> m_rawDecodeBuffer;
    util::FIFO<uint8_t>  m_decodeBuffer;
    AudioStreamBasicDescription m_iasbd, m_oasbd;
    int m_currentEdit;
    int64_t m_currentEditEndPosition;
public:
    MP4Source(std::shared_ptr<IInputStream> stream);
    uint64_t length() const
    {
        return m_edits.totalDuration();
    }
    const AudioStreamBasicDescription &getSampleFormat() const
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
    const std::vector<misc::chapter_t> &getChapters() const
    {
        return m_chapters;
    }
private:
    bool readPacket(std::vector<uint8_t> *buffer);
    void fillDecodeBuffer();
    void setupALAC();
    void setupFLAC();
    void setupMPEG4Audio();
    void setupOpus();
    unsigned getMaxFrameDependency();
    unsigned getDecoderDelay();
};
