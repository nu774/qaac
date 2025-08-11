#ifndef CAFSOURCE_H
#define CAFSOURCE_H

#include "ISource.h"
#include "PacketDecoder.h"
#include "IInputStream.h"
#include "CAFFile.h"
#include "util.h"

class CAFSource: public ISeekableSource, public ITagParser
{
    int64_t  m_position, m_position_raw;
    int64_t  m_currentPacket;
    unsigned m_start_skip;
    unsigned m_packetsPerChunk;
    std::shared_ptr<CAFFile> m_file;
    std::shared_ptr<IPacketDecoder>    m_decoder;
    std::map<std::string, std::string> m_tags;
    std::vector<uint32_t> m_chanmap;
    std::vector<uint8_t> m_packetBuffer;
    std::vector<uint8_t> m_rawDecodeBuffer;
    util::FIFO<uint8_t>  m_decodeBuffer;
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
    void seekTo(int64_t count);
    const std::map<std::string, std::string> &getTags() const { return m_tags; }
private:
    bool readPacket(std::vector<uint8_t> *buffer);
    void fillDecodeBuffer();
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