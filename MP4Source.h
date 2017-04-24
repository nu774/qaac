#include <numeric>
#include "iointer.h"
#include "mp4v2wrapper.h"
#include "PacketDecoder.h"
#include "win32util.h"

class MP4Edits {
    typedef std::pair<int64_t, int64_t> entry_t;
    std::vector<entry_t> m_edits;
public:
    void addEntry(int64_t offset, int64_t duration)
    {
        m_edits.push_back(std::make_pair(offset, duration));
    }
    size_t count() const { return m_edits.size(); }
    uint64_t totalDuration() const
    {
        return std::accumulate(m_edits.begin(), m_edits.end(), 0ULL,
                               [](uint64_t n, const entry_t &e) -> uint64_t {
                                    return n + e.second;
                               });
    }
    int64_t mediaOffset(unsigned edit_index) const
    {
        return m_edits[edit_index].first;
    }
    int64_t duration(unsigned edit_index) const
    {
        return m_edits[edit_index].second;
    }
    unsigned editForPosition(int64_t position, int64_t *offset_in_edit) const;

    int64_t mediaOffsetForPosition(int64_t position) const
    {
        int64_t  off;
        unsigned edit = editForPosition(position, &off);
        return mediaOffset(edit) + off;
    }
    void scaleShift(double ratio)
    {
        std::for_each(m_edits.begin(), m_edits.end(), [&](entry_t & e) {
                      e.first = static_cast<int64_t>(e.first * ratio + .5);
                      e.second = static_cast<int64_t>(e.second * ratio + .5);
                      });
    }
    void shiftMediaOffset(int val)
    {
        std::for_each(m_edits.begin(), m_edits.end(), [val](entry_t & e) {
                      e.first = std::max(e.first + val, (int64_t)0);
                      });
    }
};

class MP4Source: public ISeekableSource, public ITagParser,
    public IPacketFeeder, public IChapterParser
{
    uint32_t m_track_id;
    int64_t  m_position, m_position_raw;
    int64_t  m_current_packet;
    unsigned m_start_skip;
    std::shared_ptr<IPacketDecoder>    m_decoder;
    std::map<std::string, std::string> m_tags;
    std::vector<chapters::entry_t>     m_chapters;
    std::vector<uint32_t> m_chanmap;
    std::shared_ptr<FILE> m_fp;
    MP4FileX m_file;
    MP4Edits m_edits;
    std::vector<uint8_t> m_packet_buffer;
    util::FIFO<uint8_t>  m_decode_buffer;
    AudioStreamBasicDescription m_iasbd, m_oasbd;
    double m_time_ratio;
public:
    MP4Source(const std::shared_ptr<FILE> &fp);
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
    bool isSeekable() { return win32::is_seekable(fileno(m_fp.get())); }
    void seekTo(int64_t count);
    const std::map<std::string, std::string> &getTags() const { return m_tags; }
    const std::vector<chapters::entry_t> &getChapters() const
    {
        return m_chapters;
    }
    bool feed(std::vector<uint8_t> *buffer);
private:
    void setupALAC();
    void setupMPEG4Audio();
    unsigned getMaxFrameDependency();
    unsigned getDecoderDelay();
};
