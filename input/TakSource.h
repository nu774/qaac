#ifndef _TAKSRC_H
#define _TAKSRC_H

#include <tak_deco_lib.h>
#include "ISource.h"
#include "dl.h"
#include "IInputStream.h"

class TakModule {
    DL m_dl;
    bool m_compatible;
private:
    TakModule(): m_compatible(false) {
        load(L"tak_deco_lib.dll");
    }
    TakModule(const TakModule&);
    TakModule& operator=(const TakModule&);
public:
    static TakModule &instance() {
        static TakModule self;
        return self;
    }
    bool load(const std::wstring &path);
    bool loaded() const { return m_dl.loaded(); }
    bool compatible() const { return m_compatible; }

    TtakResult (*GetLibraryVersion)(TtakInt32 *, TtakInt32 *);
    TtakSeekableStreamDecoder
        (*SSD_Create_FromStream)(const TtakStreamIoInterface *, void *,
                                 const TtakSSDOptions *, TSSDDamageCallback,
                                 void *);
    void (*SSD_Destroy)(TtakSeekableStreamDecoder);
    TtakResult (*SSD_GetStreamInfo)(TtakSeekableStreamDecoder,
                                    Ttak_str_StreamInfo *);
    TtakResult (*SSD_GetStreamInfo_V22)(TtakSeekableStreamDecoder,
                                        Ttak_str_StreamInfo_V22 *);
    TtakResult (*SSD_Seek)(TtakSeekableStreamDecoder, TtakInt64);
    TtakResult (*SSD_ReadAudio)(TtakSeekableStreamDecoder, void *,
                                TtakInt32, TtakInt32 *);
    TtakInt64 (*SSD_GetReadPos)(TtakSeekableStreamDecoder);
};

class TakSource: public ISeekableSource, public ITagParser {
    uint32_t m_block_align;
    uint64_t m_length;
    std::shared_ptr<void> m_decoder;
    std::shared_ptr<IInputStream> m_stream;
    std::vector<uint32_t> m_chanmap;
    std::map<std::string, std::string> m_tags;
    std::vector<uint8_t> m_buffer;
    AudioStreamBasicDescription m_asbd;
    TakModule &m_module;
public:
    TakSource(std::shared_ptr<IInputStream> stream);
    ~TakSource() { m_decoder.reset(); }
    uint64_t length() const { return m_length; }
    const AudioStreamBasicDescription &getSampleFormat() const
    {
        return m_asbd;
    }
    const std::vector<uint32_t> *getChannels() const
    {
        return m_chanmap.size() ? &m_chanmap : 0;
    }
    int64_t getPosition();
    size_t readSamples(void *buffer, size_t nsamples);
    void seekTo(int64_t count);
    const std::map<std::string, std::string> &getTags() const { return m_tags; }
private:
    void fetchTags();
    static void staticDamageCallback(void *ctx, PtakSSDDamageItem info)
    {
        throw std::runtime_error("TAK: damaged frame found");
    }
};

#endif
