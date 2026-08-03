#include "OggIndex.h"
#include <cstdio>
#include <cstring>
#include <map>
#include <stdexcept>
#include <ogg/ogg.h>

namespace {
    std::string identifyCodec(const std::vector<uint8_t> &packet)
    {
        if (packet.size() >= 8 && !std::memcmp(packet.data(), "OpusHead", 8))
            return "opus";
        if (packet.size() >= 5 && packet[0] == 0x7f
                && !std::memcmp(packet.data() + 1, "FLAC", 4))
            return "flac";
        if (packet.size() >= 7 && packet[0] == 1
                && !std::memcmp(packet.data() + 1, "vorbis", 6))
            return "vorbis";
        return "";
    }
}

void OggIndex::build(const std::shared_ptr<IInputStream> &stream)
{
    if (!stream->seekable())
        throw std::runtime_error("Ogg: input stream must be seekable");
    stream->seek(0, SEEK_SET);

    m_chains.clear();
    std::map<uint32_t, size_t> chainForSerial;   // serial -> index into m_chains
    std::map<uint32_t, ogg_stream_state> pending; // serial -> demux state, only while
                                                  // still waiting for the id header packet

    ogg_sync_state oy;
    ogg_sync_init(&oy);
    int64_t bufferStart = 0; // file offset of the first unconsumed byte in oy

    const long kChunk = 8192;
    for (;;) {
        char *buf = ogg_sync_buffer(&oy, kChunk);
        int n = stream->read(buf, kChunk);
        if (n <= 0)
            break;
        ogg_sync_wrote(&oy, n);

        ogg_page og;
        int rc;
        while ((rc = ogg_sync_pageout(&oy, &og)) != 0) {
            if (rc < 0)
                continue; // gap in the data; keep scanning from here
            int64_t pageOffset = bufferStart;
            bufferStart += og.header_len + og.body_len;

            uint32_t serial = static_cast<uint32_t>(ogg_page_serialno(&og));
            if (ogg_page_bos(&og)) {
                m_chains.emplace_back();
                OggChainInfo &chain = m_chains.back();
                chain.serial = serial;
                chain.first_page_offset = pageOffset;
                chainForSerial[serial] = m_chains.size() - 1;
                ogg_stream_state &os = pending[serial];
                ogg_stream_init(&os, serial);
            }
            auto cit = chainForSerial.find(serial);
            if (cit == chainForSerial.end())
                continue; // page for a stream we never saw a BOS for; ignore
            OggChainInfo &chain = m_chains[cit->second];
            /*
             * Only keep pages that are safe, informative seek anchors:
             * not a lacing continuation (so a fresh packet genuinely
             * starts here) and with a granule position that actually
             * reflects decoded audio (id/comment header pages report 0
             * or -1, since no real packet completes on them -- keeping
             * those would make a seek near the start land back on a
             * header page instead of decoding forward from it).
             */
            if (!ogg_page_continued(&og) && ogg_page_granulepos(&og) > 0)
                chain.page_index.emplace_back(ogg_page_granulepos(&og), pageOffset);
            if (ogg_page_granulepos(&og) != -1)
                chain.total_samples = ogg_page_granulepos(&og);

            auto sit = pending.find(serial);
            if (sit != pending.end()) {
                /*
                 * Only feed pages into the stream demuxer until the id
                 * header packet (always the sole packet of the BOS page
                 * for the mappings we care about) comes out. Once we
                 * have it, drop the stream state immediately -- if we
                 * kept feeding every page for the whole file, libogg
                 * would keep buffering packet data we never drain.
                 */
                ogg_stream_pagein(&sit->second, &og);
                ogg_packet op;
                if (ogg_stream_packetout(&sit->second, &op) > 0) {
                    chain.id_header_packet.assign(op.packet, op.packet + op.bytes);
                    chain.codec = identifyCodec(chain.id_header_packet);
                    ogg_stream_clear(&sit->second);
                    pending.erase(sit);
                }
            }
        }
    }
    for (auto &kv: pending)
        ogg_stream_clear(&kv.second);
    ogg_sync_clear(&oy);

    if (m_chains.empty())
        throw std::runtime_error("Not a valid Ogg file");
}
