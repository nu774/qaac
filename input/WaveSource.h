#ifndef WaveSource_H
#define WaveSource_H

#include "ISource.h"
#include "cautil.h"
#include "win32util.h"
#include "IInputStream.h"

namespace wave {
    struct GUID {
        uint32_t Data1;
        uint16_t Data2;
        uint16_t Data3;
        uint8_t  Data4[8];
    };
    extern const GUID ksFormatSubTypePCM;
    extern const GUID ksFormatSubTypeFloat;
}

class WaveSource: public ISeekableSource {
    int m_block_align;
    int64_t m_data_pos;
    int64_t m_position;
    uint64_t m_length;
    std::shared_ptr<IInputStream> m_stream;
    std::vector<uint32_t> m_chanmap;
    std::vector<uint8_t> m_buffer;
    AudioStreamBasicDescription m_asbd;
public:
    WaveSource(std::shared_ptr<IInputStream> m_stream, bool ignorelength = false);
    uint64_t length() const { return m_length; }
    const AudioStreamBasicDescription &getSampleFormat() const
    {
        return m_asbd;
    }
    const std::vector<uint32_t> *getChannels() const
    {
        return m_chanmap.size() ? &m_chanmap : 0;
    }
    int64_t getPosition() { return m_position; }
    size_t readSamples(void *buffer, size_t nsamples);
    void seekTo(int64_t count);
private:
    int64_t parse();
    void read16le(void *n);
    void read32le(void *n);
    void read64le(void *n);
    uint32_t nextChunk(uint32_t *size);
    int64_t ds64();
    void fmt(size_t size);
};

#endif
