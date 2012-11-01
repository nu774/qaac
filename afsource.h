#ifndef AFSOURCE_H
#define AFSOURCE_H

#include "AudioFileX.h"
#include "ExtAudioFileX.h"
#include "ioabst.h"
#include "iointer.h"

std::shared_ptr<ISource>
AudioFileOpenFactory(InputStream &stream, const std::wstring &path);

class AFSource: public ITagParser, public PartialSource<AFSource>
{
    AudioFileX m_af;
    uint64_t m_offset;
    std::shared_ptr<InputStream> m_stream;
    std::vector<uint32_t> m_chanmap;
    std::map<uint32_t, std::wstring> m_tags;
    AudioStreamBasicDescription m_asbd;
public:
    AFSource(AudioFileX &af, std::shared_ptr<InputStream> &stream);
    /*
     * XXX: AudioFile_GetSizeProc is called inside of AudioFileClose().
     * Therefore, we must first call AudioFileClose() before destruction.
     */
    ~AFSource() { m_af.attach(0, false); }
    uint64_t length() const { return getDuration(); }
    const AudioStreamBasicDescription &getSampleFormat() const
    {
	return m_asbd;
    }
    const std::vector<uint32_t> *getChannels() const
    {
	return m_chanmap.size() ? &m_chanmap: 0;
    }
    size_t readSamples(void *buffer, size_t nsamples);
    void skipSamples(int64_t count) { m_offset += count; }
    const std::map<uint32_t, std::wstring> &getTags() const { return m_tags; }
    const std::vector<std::pair<std::wstring, int64_t> >
	*getChapters() const { return 0; }
};

class ExtAFSource: public ITagParser, public PartialSource<ExtAFSource>
{
    AudioFileX m_af;
    ExtAudioFileX m_eaf;
    uint64_t m_offset;
    std::shared_ptr<InputStream> m_stream;
    std::vector<uint32_t> m_chanmap;
    std::map<uint32_t, std::wstring> m_tags;
    AudioStreamBasicDescription m_asbd;
public:
    ExtAFSource(AudioFileX &af, std::shared_ptr<InputStream> &stream,
		const std::wstring &path);
    ~ExtAFSource()
    {
	m_af.attach(0, false);
	m_eaf.attach(0, false);
    }
    uint64_t length() const { return getDuration(); }
    const AudioStreamBasicDescription &getSampleFormat() const
    {
	return m_asbd;
    }
    const std::vector<uint32_t> *getChannels() const
    {
	return m_chanmap.size() ? &m_chanmap: 0;
    }
    size_t readSamples(void *buffer, size_t nsamples);
    void skipSamples(int64_t count);
    const std::map<uint32_t, std::wstring> &getTags() const { return m_tags; }
    const std::vector<std::pair<std::wstring, int64_t> >
	*getChapters() const { return 0; }
};
#endif
