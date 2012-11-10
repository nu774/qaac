#include <ALACDecoder.h>
#include "iointer.h"
#include "mp4v2wrapper.h"

class ALACSource: public ISeekableSource, public ITagParser
{
    struct DecodeBuffer {
	uint32_t nsamples;
	uint32_t done;
	uint32_t bytes_per_frame;
	std::vector<uint8_t> v;

	DecodeBuffer(): nsamples(0), done(0), bytes_per_frame(0) {}
	void resize(uint32_t nsamples)
	{
	    size_t n = nsamples * bytes_per_frame;
	    if (n > v.size()) v.resize(n);
	}
	uint8_t *read_ptr() { return &v[done * bytes_per_frame]; }
	uint8_t *write_ptr() { return &v[0]; }
	void reset() { nsamples = done = 0; }
	uint32_t count() { return nsamples - done; }
	void advance(uint32_t n) {
	    done += n;
	    if (done >= nsamples) done = nsamples = 0;
	}
	void commit(uint32_t count, uint32_t read_pos)
	{
	    nsamples = count;
	    done = read_pos;
	}
    };
    uint32_t m_track_id;
    uint64_t m_length;
    int64_t m_position;
    std::shared_ptr<ALACDecoder> m_decoder;
    std::map<uint32_t, std::wstring> m_tags;
    std::vector<uint32_t> m_chanmap;
    MP4FileX m_file;
    DecodeBuffer m_buffer;
    std::shared_ptr<FILE> m_fp;
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
