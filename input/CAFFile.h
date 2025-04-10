#ifndef CAFFILE_H
#define CAFFILE_H

#include <memory>
#include <cstring>
#include <cstdint>
#include <vector>
#include <string>
#include <utility>
#include "CoreAudio/CoreAudioTypes.h"
#include "CoreAudio/AudioFile.h"
#include "IInputStream.h"

class CAFFile {
public:
    struct Format {
        AudioStreamBasicDescription  asbd;
        std::vector<uint32_t> channel_layout;
        Format() { std::memset(&asbd, 0, sizeof asbd); }
    };
private:
    std::shared_ptr<IInputStream>                     m_stream;
    std::vector<std::pair<std::string, std::string> > m_tags;
    Format                                            m_primary_format;
    std::vector<Format>                               m_layered_formats;
    std::vector<uint8_t>                              m_magic_cookie;
    AudioFilePacketTableInfo                          m_packet_info;
    std::vector<AudioStreamPacketDescription>         m_packet_table;
    int64_t                                           m_data_offset;
    int64_t                                           m_data_size;
    int64_t                                           m_duration;
public:
    CAFFile(const std::shared_ptr<IInputStream> &stream)
        : m_stream(stream)
        , m_data_offset(0)
        , m_data_size(0)
        , m_duration(0)
    {
        memset(&m_packet_info, 0, sizeof m_packet_info);
        parse();
    }
    const Format &format() const
    {
        return m_layered_formats.size() ? m_layered_formats[0]
                                        : m_primary_format;
    }
    const Format &primary_format() const
    {
        return m_primary_format;
    }
    const std::vector<uint32_t> channel_layout() const
    {
        return format().channel_layout;
    }
    int64_t duration() const /* in number of PCM frames */
    {
        return m_duration;
    }
    int64_t num_packets() const
    {
        if (m_packet_table.size())
            return m_packet_table.size();
        else
            return m_data_size < 0 ? INT64_MAX : m_data_size / format().asbd.mBytesPerPacket;
    }
    int32_t start_offset() const
    {
        return m_packet_info.mPrimingFrames * tscale() + .5;
    }
    uint32_t end_padding() const
    {
        return m_packet_info.mRemainderFrames * tscale() + .5;
    }
    void get_magic_cookie(std::vector<uint8_t> *data) const;

    /* returns position in bytes, optionally fills packet size */
    int64_t packet_info(int64_t index, uint32_t *size=0) const;

    uint32_t read_packets(int64_t offset, uint32_t count,
                          std::vector<uint8_t> *data);

    const std::vector<std::pair<std::string, std::string>> &get_tags() const
    {
        return m_tags;
    }

private:
    CAFFile(const CAFFile &);
    CAFFile& operator=(const CAFFile &);

    void read16be(void *n);
    void read32be(void *n);
    void read64be(void *n);
    uint32_t nextChunk(uint64_t *size);

    double tscale() const
    {
        if (!m_layered_formats.size())
            return 1.0;
        else
            return format().asbd.mSampleRate / m_primary_format.asbd.mSampleRate;
    }
    void parse();
    void parse_desc(Format *d);
    void parse_chan(Format *d, int64_t size);
    void parse_ldsc(int64_t size);
    void parse_kuki(int64_t size);
    void parse_info(int64_t size);
    void parse_pakt(int64_t size);
    void calc_duration();
};

#endif
