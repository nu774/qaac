#ifndef _TAKSRC_H
#define _TAKSRC_H

#include <tak_deco_lib.h>
#include "iointer.h"
#include "dl.h"

struct TtakAudioFormatEx: public TtakAudioFormat {
    TtakBool  HasExtension;
    TtakInt32 ValidBitsPerSample;
    TtakBool  HasSpeakerAssignment;
    char SpeakerAssignment[16];
};

struct Ttak_str_StreamInfo_V10 {
    Ttak_str_EncoderInfo Encoder;
    Ttak_str_SizeInfo    Sizes;
    TtakAudioFormat      Audio;
};

struct Ttak_str_StreamInfo_V22 {
    Ttak_str_EncoderInfo Encoder;
    Ttak_str_SizeInfo    Sizes;
    TtakAudioFormatEx    Audio;
};

class TakModule {
    DL m_dl;
    bool m_compatible;
public:
    TakModule() {}
    explicit TakModule(const std::wstring &path);
    bool loaded() const { return m_dl.loaded(); }
    bool compatible() const { return m_compatible; }

    TtakResult (*GetLibraryVersion)(TtakInt32 *, TtakInt32 *);
    TtakSeekableStreamDecoder
        (*SSD_Create_FromStream)(const TtakStreamIoInterface *, void *,
                                 const TtakSSDOptions *, TSSDDamageCallback,
                                 void *);
    void (*SSD_Destroy)(TtakSeekableStreamDecoder);
    TtakResult (*SSD_GetStreamInfo)(TtakSeekableStreamDecoder,
                                    Ttak_str_StreamInfo_V10 *);
    TtakResult (*SSD_GetStreamInfo_V22)(TtakSeekableStreamDecoder,
                                        Ttak_str_StreamInfo_V22 *);
    TtakResult (*SSD_Seek)(TtakSeekableStreamDecoder, TtakInt64);
    TtakResult (*SSD_ReadAudio)(TtakSeekableStreamDecoder, void *,
                                TtakInt32, TtakInt32 *);
    TtakInt64 (*SSD_GetReadPos)(TtakSeekableStreamDecoder);
};

class TakSource: public ISeekableSource, public ITagParser
{
    uint32_t m_block_align;
    uint64_t m_length;
    std::shared_ptr<void> m_decoder;
    std::shared_ptr<FILE> m_fp;
    std::vector<uint32_t> m_chanmap;
    std::map<std::string, std::string> m_tags;
    std::vector<chapters::entry_t> m_chapters;
    std::vector<uint8_t> m_buffer;
    AudioStreamBasicDescription m_asbd;
    TakModule m_module;
public:
    TakSource(const TakModule &module, const std::shared_ptr<FILE> &fp);
    uint64_t length() const { return m_length; }
    const AudioStreamBasicDescription &getSampleFormat() const
    {
        return m_asbd;
    }
    const std::vector<uint32_t> *getChannels() const { return &m_chanmap; }
    int64_t getPosition();
    size_t readSamples(void *buffer, size_t nsamples);
    bool isSeekable() { return util::is_seekable(fileno(m_fp.get())); }
    void seekTo(int64_t count);
    const std::map<std::string, std::string> &getTags() const { return m_tags; }
    const std::vector<chapters::entry_t> *getChapters() const
    {
        return m_chapters.size() ? &m_chapters : 0;
    }
private:
    void fetchTags();
    static void staticDamageCallback(void *ctx, PtakSSDDamageItem info)
    {
        throw std::runtime_error("TAK: damaged frame found");
    }
};

#endif
