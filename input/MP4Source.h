#include <numeric>
#include "ISource.h"
#include "mp4v2wrapper.h"
#include "PacketDecoder.h"
#include "win32util.h"
#include "IInputStream.h"
#include "MP4Edits.h"

class MP4Source: public ISeekableSource, public ITagParser,
    public IPacketFeeder, public IChapterParser
{
    uint32_t m_track_id;
    int64_t  m_position, m_position_raw;
    int64_t  m_current_packet;
    unsigned m_start_skip;
    std::shared_ptr<IPacketDecoder>    m_decoder;
    std::map<std::string, std::string> m_tags;
    std::vector<misc::chapter_t>     m_chapters;
    std::vector<uint32_t> m_chanmap;
    std::shared_ptr<IInputStream> m_stream;
    MP4FileX m_file;
    MP4Edits m_edits;
    std::vector<uint8_t> m_packet_buffer;
    util::FIFO<uint8_t>  m_decode_buffer;
    AudioStreamBasicDescription m_iasbd, m_oasbd;
    double m_time_ratio;
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
    bool feed(std::vector<uint8_t> *buffer);
private:
    void setupALAC();
    void setupFLAC();
    void setupMPEG4Audio();
    void setupOpus();
    unsigned getMaxFrameDependency();
    unsigned getDecoderDelay();
};
