#include <ALACDecoder.h>
#include "iointer.h"
#include "mp4v2wrapper.h"

class ALACSource: public ISeekableSource, public ITagParser
{
    uint32_t m_track_id;
    uint64_t m_length;
    int64_t m_position;
    std::shared_ptr<ALACDecoder> m_decoder;
    std::map<uint32_t, std::wstring> m_tags;
    std::vector<uint32_t> m_chanmap;
    std::shared_ptr<FILE> m_fp;
    MP4FileX m_file;
    DecodeBuffer<uint8_t> m_buffer;
    AudioStreamBasicDescription m_asbd, m_oasbd;
public:
    ALACSource(const std::shared_ptr<FILE> &fp);
    uint64_t length() const { return m_length; }
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
    bool isSeekable() { return util::is_seekable(fileno(m_fp.get())); }
    void seekTo(int64_t count)
    {
        m_position = count;
        m_buffer.reset();
    }
    const std::map<uint32_t, std::wstring> &getTags() const { return m_tags; }
    const std::vector<chapters::entry_t> *getChapters() const { return 0; }
};
