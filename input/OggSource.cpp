#include "OggSource.h"
#include <algorithm>
#include <cstring>
#include <stdexcept>
#include "OpusPacketDecoder.h"
#include "FLACPacketDecoder.h"
#include "VorbisPacketDecoder.h"
#include "taglibhelper.h"
#include "strutil.h"
#include "metadata.h"
#include <opusfile.h>
#include <oggflacfile.h>
#include <vorbisfile.h>

namespace {
    class WindowedInputStream: public IInputStream {
        std::shared_ptr<IInputStream> m_stream;
        int64_t m_start, m_size;
    public:
        WindowedInputStream(std::shared_ptr<IInputStream> stream,
                            int64_t start, int64_t end)
            : m_stream(stream), m_start(start), m_size(end - start)
        {
            m_stream->seek(m_start, SEEK_SET);
        }
        bool seekable() override { return m_stream->seekable(); }
        int read(void *buf, unsigned size) override
        {
            int64_t pos = m_stream->tell() - m_start;
            if (pos < 0 || pos >= m_size)
                return 0;
            if (static_cast<int64_t>(size) > m_size - pos)
                size = static_cast<unsigned>(m_size - pos);
            return m_stream->read(buf, size);
        }
        int64_t seek(int64_t off, int whence) override
        {
            int64_t target;
            switch (whence) {
            case SEEK_CUR: target = (m_stream->tell() - m_start) + off; break;
            case SEEK_END: target = m_size + off; break;
            default:       target = off; break;
            }
            if (target < 0) target = 0;
            if (target > m_size) target = m_size;
            return m_stream->seek(m_start + target, SEEK_SET) - m_start;
        }
        int64_t tell() override { return m_stream->tell() - m_start; }
        int64_t size() override { return m_size; }
    };

    /*
     * OpusPacketDecoder::setMagicCookie() expects the ISOBMFF 'dOps' box
     * layout (big-endian multi-byte fields), but the raw Ogg id header
     * packet ("OpusHead", RFC 7845) uses little-endian fields. Repack.
     */
    std::vector<uint8_t> opusHeadToCookie(const std::vector<uint8_t> &packet,
                                          uint16_t *preSkip)
    {
        if (packet.size() < 19 || std::memcmp(packet.data(), "OpusHead", 8))
            throw std::runtime_error("Malformed OpusHead packet");
        uint8_t version = packet[8];
        uint8_t channels = packet[9];
        uint16_t pre_skip = packet[10] | (packet[11] << 8);
        uint32_t rate = packet[12] | (packet[13] << 8) |
                        (packet[14] << 16) | (packet[15] << 24);
        uint16_t gain = packet[16] | (packet[17] << 8);
        uint8_t mapping_family = packet[18];

        std::vector<uint8_t> cookie;
        cookie.push_back(version);
        cookie.push_back(channels);
        cookie.push_back((pre_skip >> 8) & 0xff);
        cookie.push_back(pre_skip & 0xff);
        cookie.push_back((rate >> 24) & 0xff);
        cookie.push_back((rate >> 16) & 0xff);
        cookie.push_back((rate >> 8) & 0xff);
        cookie.push_back(rate & 0xff);
        cookie.push_back((gain >> 8) & 0xff);
        cookie.push_back(gain & 0xff);
        cookie.push_back(mapping_family);
        if (mapping_family != 0) {
            if (packet.size() < 21u + channels)
                throw std::runtime_error("Malformed OpusHead packet "
                                         "(channel mapping truncated)");
            cookie.push_back(packet[19]);
            cookie.push_back(packet[20]);
            for (int i = 0; i < channels; ++i)
                cookie.push_back(packet[21 + i]);
        }
        *preSkip = pre_skip;
        return cookie;
    }

    /*
     * FLACPacketDecoder::setMagicCookie() expects raw native FLAC
     * metadata blocks (as in the ISOBMFF 'dfLa' box, sans its 4-byte
     * FullBox header). The Ogg FLAC id header packet is
     * 0x7F "FLAC" major minor n_header_packets(2) "fLaC" <same blocks>,
     * a 13-byte preamble followed by exactly that.
     */
    std::vector<uint8_t> oggFlacHeaderToCookie(const std::vector<uint8_t> &packet)
    {
        if (packet.size() < 13 || packet[0] != 0x7f
                || std::memcmp(packet.data() + 1, "FLAC", 4))
            throw std::runtime_error("Malformed Ogg FLAC id header packet");
        std::vector<uint8_t> cookie(packet.begin() + 13, packet.end());
        /*
         * In the Ogg mapping, STREAMINFO's "last metadata block" flag is
         * 0 -- the comment header (and possibly more) logically follows,
         * just as a separate Ogg packet rather than a contiguous native
         * FLAC metadata block. We're intentionally feeding the decoder
         * only STREAMINFO here, so force the flag: without it, libFLAC's
         * native stream decoder expects another block and errors out on
         * hitting the end of what we gave it.
         */
        if (!cookie.empty())
            cookie[0] |= 0x80;
        return cookie;
    }

    /*
     * VorbisPacketDecoder::setMagicCookie() needs all three Vorbis header
     * packets (id, comment, setup) to call vorbis_synthesis_headerin() --
     * unlike Opus/FLAC's single-header cookies, since the setup packet
     * carries the codebooks that decoding depends on. Concatenate them as
     * three [4-byte BE length][data] blocks, in order.
     */
    std::vector<uint8_t> vorbisHeadersToCookie(const OggChainInfo &chain)
    {
        std::vector<uint8_t> cookie;
        const std::vector<uint8_t> *packets[3] = {
            &chain.id_header_packet,
            &chain.comment_header_packet,
            &chain.setup_header_packet,
        };
        for (auto *packet: packets) {
            uint32_t len = static_cast<uint32_t>(packet->size());
            cookie.push_back((len >> 24) & 0xff);
            cookie.push_back((len >> 16) & 0xff);
            cookie.push_back((len >> 8) & 0xff);
            cookie.push_back(len & 0xff);
            cookie.insert(cookie.end(), packet->begin(), packet->end());
        }
        return cookie;
    }
}

OggSource::OggSource(std::shared_ptr<IInputStream> stream,
                     std::shared_ptr<const std::vector<OggChainInfo>> index,
                     size_t chainIndex)
    : m_stream(stream), m_index(index), m_chainIndex(chainIndex),
      m_preSkip(0), m_totalSamples(0), m_headerPacketCount(0),
      m_scanForLastMetadataBlock(false), m_prerollPackets(0),
      m_streamInited(false), m_eos(false), m_position(0)
{
    memset(&m_oy, 0, sizeof m_oy);
    memset(&m_os, 0, sizeof m_os);
    memset(&m_oasbd, 0, sizeof m_oasbd);

    const OggChainInfo &c = chain();
    if (c.codec == "opus") {
        uint16_t preSkip;
        auto cookie = opusHeadToCookie(c.id_header_packet, &preSkip);
        auto decoder = std::make_shared<OpusPacketDecoder>();
        decoder->setMagicCookie(cookie);
        m_decoder = decoder;
        m_preSkip = preSkip;
        m_headerPacketCount = 1; // OpusTags, always exactly one (RFC 7845)
        m_prerollPackets = 4;    // matches MP4Source::getMaxFrameDependency() for opus
    } else if (c.codec == "flac") {
        auto cookie = oggFlacHeaderToCookie(c.id_header_packet);
        auto decoder = std::make_shared<FLACPacketDecoder>();
        decoder->setMagicCookie(cookie);
        m_decoder = decoder;
        m_preSkip = 0;
        m_scanForLastMetadataBlock = true;
        m_prerollPackets = 0; // FLAC frames are independently decodable
    } else if (c.codec == "vorbis") {
        auto cookie = vorbisHeadersToCookie(c);
        auto decoder = std::make_shared<VorbisPacketDecoder>();
        decoder->setMagicCookie(cookie);
        m_decoder = decoder;
        m_preSkip = 0;
        m_headerPacketCount = 2; // comment + setup, already folded into the cookie above
        m_prerollPackets = 1; // see the granule resync in seekTo()'s warm-up loop
    } else {
        throw std::runtime_error("Unsupported Ogg codec: " + c.codec);
    }
    m_oasbd = m_decoder->getSampleFormat();
    m_decodeBuffer.set_unit(m_oasbd.mBytesPerFrame);
    m_totalSamples = (std::max)(int64_t(0), c.total_samples - m_preSkip);

    fetchTags();
    seekTo(0);
}

OggSource::~OggSource()
{
    if (m_streamInited) {
        ogg_stream_clear(&m_os);
        ogg_sync_clear(&m_oy);
    }
}

void OggSource::restartAt(int64_t byteOffset)
{
    if (m_streamInited) {
        ogg_stream_clear(&m_os);
        ogg_sync_clear(&m_oy);
    }
    m_stream->seek(byteOffset, SEEK_SET);
    ogg_sync_init(&m_oy);
    ogg_stream_init(&m_os, chain().serial);
    m_streamInited = true;
    m_eos = false;

    if (byteOffset == chain().first_page_offset) {
        std::vector<uint8_t> dummy;
        readPacket(&dummy); // id header
        if (m_scanForLastMetadataBlock) {
            bool last = false;
            while (!last) {
                if (!readPacket(&dummy))
                    break;
                last = !dummy.empty() && (dummy[0] & 0x80);
            }
        } else {
            for (unsigned i = 0; i < m_headerPacketCount; ++i)
                readPacket(&dummy); // Opus/Vorbis: fixed header packet count
        }
    }
}

bool OggSource::readPacket(std::vector<uint8_t> *buffer, int64_t *granulepos)
{
    ogg_packet op;
    for (;;) {
        int rc = ogg_stream_packetout(&m_os, &op);
        if (rc == 1) {
            buffer->assign(op.packet, op.packet + op.bytes);
            if (granulepos)
                *granulepos = op.granulepos;
            return true;
        }
        if (rc < 0)
            continue; // hole in the packet data; keep draining
        if (m_eos)
            return false;

        ogg_page og;
        int prc;
        for (;;) {
            prc = ogg_sync_pageout(&m_oy, &og);
            if (prc != 0)
                break;
            char *buf = ogg_sync_buffer(&m_oy, 8192);
            int n = m_stream->read(buf, 8192);
            if (n <= 0) {
                m_eos = true;
                return false;
            }
            ogg_sync_wrote(&m_oy, n);
        }
        if (prc < 0)
            continue; // gap; try pageout again
        if (static_cast<uint32_t>(ogg_page_serialno(&og)) != chain().serial)
            continue; // another chain's page; not ours to read
        ogg_stream_pagein(&m_os, &og);
        if (ogg_page_eos(&og))
            m_eos = true;
    }
}

void OggSource::fillDecodeBuffer()
{
    while (m_decodeBuffer.count() == 0) {
        if (!readPacket(&m_packetBuffer))
            break;
        int nsamples = m_decoder->decode(m_packetBuffer, &m_rawDecodeBuffer);
        if (nsamples > 0) {
            m_decodeBuffer.reserve(nsamples);
            std::memcpy(m_decodeBuffer.write_ptr(), m_rawDecodeBuffer.data(),
                       m_rawDecodeBuffer.size());
            m_decodeBuffer.commit(nsamples);
        }
    }
}

size_t OggSource::readSamples(void *buffer, size_t nsamples)
{
    if (m_position >= m_totalSamples)
        return 0;
    if (m_decodeBuffer.count() == 0)
        fillDecodeBuffer();
    if (nsamples > m_decodeBuffer.count())
        nsamples = m_decodeBuffer.count();
    if (m_position + static_cast<int64_t>(nsamples) > m_totalSamples)
        nsamples = static_cast<size_t>(m_totalSamples - m_position);
    if (nsamples > 0) {
        std::memcpy(buffer, m_decodeBuffer.read(nsamples),
                   nsamples * m_oasbd.mBytesPerFrame);
        m_position += nsamples;
    }
    return nsamples;
}

void OggSource::seekTo(int64_t count)
{
    if (count < 0) count = 0;
    if (count > m_totalSamples) count = m_totalSamples;
    int64_t rawTarget = count + m_preSkip;

    int64_t seekOffset = chain().first_page_offset;
    int64_t baseline = 0;
    const auto &pages = chain().page_index;
    auto it = std::upper_bound(pages.begin(), pages.end(), rawTarget,
        [](int64_t target, const std::pair<int64_t, int64_t> &p) {
            return target < p.first;
        });
    if (it != pages.begin()) {
        auto chosen = it - 1; // last page whose granule <= rawTarget
        if (m_prerollPackets > 0 && chosen != pages.begin())
            --chosen;
        seekOffset = chosen->second;
        baseline = (chosen == pages.begin()) ? 0 : (chosen - 1)->first;
    }

    restartAt(seekOffset);
    m_decoder->reset();
    m_decodeBuffer.reset();

    int64_t rawPos = baseline;
    while (rawPos < rawTarget) {
        int64_t granulepos;
        if (!readPacket(&m_packetBuffer, &granulepos))
            break;
        int n = m_decoder->decode(m_packetBuffer, &m_rawDecodeBuffer);
        if (n <= 0) {
            if (granulepos != -1)
                rawPos = granulepos;
            continue;
        }
        int64_t posBefore = rawPos, posAfter = rawPos + n;
        if (granulepos != -1) {
            posAfter = granulepos;
            posBefore = granulepos - n;
        }
        if (posAfter > rawTarget) {
            int64_t skip = std::max<int64_t>(0, rawTarget - posBefore);
            int64_t keep = n - skip;
            m_decodeBuffer.reserve(keep);
            std::memcpy(m_decodeBuffer.write_ptr(),
                       m_rawDecodeBuffer.data() +
                           skip * m_oasbd.mBytesPerFrame,
                       keep * m_oasbd.mBytesPerFrame);
            m_decodeBuffer.commit(keep);
            rawPos = rawTarget;
        } else {
            rawPos = posAfter;
        }
    }
    m_position = count;
}

void OggSource::fetchTags()
{
    const std::string &codec = chain().codec;
    if (codec != "opus" && codec != "flac" && codec != "vorbis")
        return;

    int64_t start = chain().first_page_offset;
    int64_t end = (m_chainIndex + 1 < m_index->size())
        ? (*m_index)[m_chainIndex + 1].first_page_offset
        : m_stream->size();

    util::FilePositionSaver _(m_stream);
    auto windowed = std::make_shared<WindowedInputStream>(m_stream, start, end);
    TagLibX::IStreamReader reader(windowed);
    std::shared_ptr<TagLib::Ogg::File> file;
    if (codec == "opus")
        file = std::make_shared<TagLib::Ogg::Opus::File>(&reader, false);
    else if (codec == "flac")
        file = std::make_shared<TagLib::Ogg::FLAC::File>(&reader, false);
    else
        file = std::make_shared<TagLib::Ogg::Vorbis::File>(&reader, false);

    auto tag = dynamic_cast<TagLib::Ogg::XiphComment*>(file->tag());
    if (!tag)
        return;
    std::map<std::string, std::string> tags;
    auto &map = tag->fieldListMap();
    for (auto it = map.begin(); it != map.end(); ++it) {
        std::string key = it->first.toCString();
        tags[key] = it->second.toString().to8Bit(true);
    }
    m_tags = TextBasedTag::normalizeTags(tags);

    auto pics = tag->pictureList();
    for (auto it = pics.begin(); it != pics.end(); ++it) {
        if ((*it)->type() == TagLib::FLAC::Picture::FrontCover) {
            auto data = (*it)->data();
            m_tags["COVER ART"] = std::string(data.begin(), data.end());
        }
    }
}
