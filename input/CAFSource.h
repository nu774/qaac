#ifndef CAFSOURCE_H
#define CAFSOURCE_H

#include "ISource.h"
#include "PacketDecoder.h"
#include "IInputStream.h"
#include "CAFFile.h"
#include "util.h"

class CAFSource: public ISeekableSource, public ITagParser,
    public IPacketFeeder
{
    int64_t  m_position, m_position_raw;
    int64_t  m_current_packet;
    unsigned m_start_skip;
    unsigned m_packets_per_chunk;
    std::shared_ptr<CAFFile> m_file;
    std::shared_ptr<IPacketDecoder>    m_decoder;
    std::map<std::string, std::string> m_tags;
    std::vector<uint32_t> m_chanmap;
    std::vector<uint8_t> m_packet_buffer;
    util::FIFO<uint8_t>  m_decode_buffer;
    AudioStreamBasicDescription m_oasbd;
public:
    CAFSource(std::shared_ptr<IInputStream> stream);
    uint64_t length() const
    {
        auto len = m_file->duration();
        return len < 0 ? ~0uLL : len;
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
    size_t readSamplesLPCM(void *buffer, size_t nsamples);
    void seekTo(int64_t count);
    const std::map<std::string, std::string> &getTags() const { return m_tags; }
    bool feed(std::vector<uint8_t> *buffer);
private:
    void setupLPCM();
    void setupALAC();
    void setupFLAC();
    void setupMPEG1Audio();
    void setupMPEG4Audio();
    void setupOpus();
    unsigned getMaxFrameDependency();
    unsigned getDecoderDelay();
};

#endif