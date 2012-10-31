#ifndef _TAKSRC_H
#define _TAKSRC_H

#include <tak_deco_lib.h>
#include "ioabst.h"
#include "iointer.h"
#include "dl.h"

class TakModule {
    DL m_dl;
    bool m_compatible;
public:
    TakModule() {}
    explicit TakModule(const std::wstring &path);
    bool loaded() const { return m_dl.loaded(); }
    bool compatible() const { return m_compatible; }

    TtakResult (*GetLibraryVersion)(TtakInt32 *, TtakInt32 *);
    TtakSeekableStreamDecoder (*SSD_Create_FromStream)(
	    const TtakStreamIoInterface *, void *, const TtakSSDOptions *,
	    TSSDDamageCallback, void *);
    void (*SSD_Destroy)(TtakSeekableStreamDecoder);
    TtakResult (*SSD_GetStreamInfo)(TtakSeekableStreamDecoder,
	    Ttak_str_StreamInfo *);
    TtakResult (*SSD_Seek)(TtakSeekableStreamDecoder, TtakInt64);
    TtakResult (*SSD_ReadAudio)(TtakSeekableStreamDecoder, void *,
	    TtakInt32, TtakInt32 *);
    TtakAPEv2Tag (*SSD_GetAPEv2Tag)(TtakSeekableStreamDecoder);
    TtakInt32 (*APE_GetItemNum)(TtakAPEv2Tag);
    TtakResult (*APE_GetItemKey)(TtakAPEv2Tag, TtakInt32, char *,
	    TtakInt32, TtakInt32 *);
    TtakResult (*APE_GetItemValue)(TtakAPEv2Tag, TtakInt32, void *,
	    TtakInt32, TtakInt32 *);
};

class TakSource:
    public ITagParser, public PartialSource<TakSource>
{
    TakModule m_module;
    x::shared_ptr<void> m_decoder;
    InputStream m_stream;
    AudioStreamBasicDescription m_format;
    std::vector<uint32_t> m_chanmap;
    std::map<uint32_t, std::wstring> m_tags;
    std::vector<std::pair<std::wstring, int64_t> > m_chapters;
public:
    TakSource(const TakModule &module, InputStream &stream);
    uint64_t length() const { return getDuration(); }
    const AudioStreamBasicDescription &getSampleFormat() const
    {
	return m_format;
    }
    const std::vector<uint32_t> *getChannels() const { return 0; }
    size_t readSamples(void *buffer, size_t nsamples);
    void skipSamples(int64_t count);
    const std::map<uint32_t, std::wstring> &getTags() const { return m_tags; }
    const std::vector<std::pair<std::wstring, int64_t> >
	*getChapters() const
    {
	if (m_chapters.size())
	    return &m_chapters;
	else
	    return 0;
    }
private:
    void fetchTags();
    static void staticDamageCallback(void *ctx, PtakSSDDamageItem info)
    {
	throw std::runtime_error("TAK: damaged frame found");
    }
};

#endif
