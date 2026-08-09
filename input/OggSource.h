#ifndef OGGSOURCE_H
#define OGGSOURCE_H

#include <memory>
#include <ogg/ogg.h>
#include "ISource.h"
#include "IInputStream.h"
#include "PacketDecoder.h"
#include "OggIndex.h"
#include "util.h"

/*
 * One logical Ogg bitstream (chain) from an Opus- or FLAC-in-Ogg file,
 * decoded via the codec's IPacketDecoder -- the same abstraction
 * MP4Source uses for ISOBMFF-contained Opus/FLAC. Several OggSource
 * instances (one per chain) can share the same underlying stream and
 * OggIndex; each owns its own libogg demux state.
 */
class OggSource: public ISeekableSource, public ITagParser {
public:
    OggSource(std::shared_ptr<IInputStream> stream,
             std::shared_ptr<const std::vector<OggChainInfo>> index,
             size_t chainIndex);
    ~OggSource();

    uint64_t length() const override { return m_totalSamples; }
    const ca::AudioStreamBasicDescription &getSampleFormat() const override
    {
        return m_oasbd;
    }
    const std::vector<uint32_t> *getChannels() const override { return 0; }
    int64_t getPosition() override { return m_position; }
    size_t readSamples(void *buffer, size_t nsamples) override;
    void seekTo(int64_t count) override;
    const std::map<std::string, std::string> &getTags() const override
    {
        return m_tags;
    }
private:
    const OggChainInfo &chain() const { return (*m_index)[m_chainIndex]; }
    void fetchTags();
    void restartAt(int64_t byteOffset);
    bool readPacket(std::vector<uint8_t> *buffer, int64_t *granulepos = 0);
    void fillDecodeBuffer();

    std::shared_ptr<IInputStream> m_stream;
    std::shared_ptr<const std::vector<OggChainInfo>> m_index;
    size_t m_chainIndex;

    std::shared_ptr<IPacketDecoder> m_decoder;
    ca::AudioStreamBasicDescription m_oasbd;
    int64_t m_preSkip;
    int64_t m_totalSamples;      // post pre-skip/end-trim, in m_oasbd's sample rate
    unsigned m_headerPacketCount; // header packets after the id header to skip
    unsigned m_prerollPackets;    // extra decode-and-discard margin after a seek

    ogg_sync_state m_oy;
    ogg_stream_state m_os;
    bool m_streamInited;
    bool m_eos;

    int64_t m_position;

    std::vector<uint8_t> m_packetBuffer;
    std::vector<uint8_t> m_rawDecodeBuffer;
    util::FIFO<uint8_t> m_decodeBuffer;

    std::map<std::string, std::string> m_tags;
};

#endif
