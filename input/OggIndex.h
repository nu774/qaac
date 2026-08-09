#ifndef OGGINDEX_H
#define OGGINDEX_H

#include <cstdint>
#include <string>
#include <utility>
#include <vector>
#include <memory>
#include "IInputStream.h"

/*
 * One logical Ogg bitstream (a "chain" in the chained-Ogg sense: a
 * physically concatenated file can contain several of these, each with
 * its own serial number and potentially its own format).
 */
struct OggChainInfo {
    uint32_t serial = 0;
    std::string codec;                     // "opus", "flac", "vorbis", or "" if unrecognized
    int64_t first_page_offset = 0;         // byte offset of the BOS page
    int64_t total_samples = 0;             // raw granule position of the chain's last page
    std::vector<uint8_t> id_header_packet; // first packet: codec identification header

    // Vorbis only: the comment and setup header packets (2nd and 3rd
    // packets). Unlike Opus/FLAC, decoding Vorbis needs all three header
    // packets fed to vorbis_synthesis_headerin() before any audio packet
    // can be decoded -- the setup packet carries the codebooks.
    std::vector<uint8_t> comment_header_packet;
    std::vector<uint8_t> setup_header_packet;

    // Byte offset of every page belonging to this chain, in file order,
    // paired with that page's granule position. Used to seek: binary
    // search for the last page at/before a target granule, jump the
    // underlying stream there, then decode forward a short distance.
    std::vector<std::pair<int64_t, int64_t>> page_index;
};

/*
 * Scans an Ogg file once, front to back, and records enough about each
 * logical bitstream to seek and decode it without ever needing to
 * bisect: page granule positions are cheap to read from the page header,
 * so indexing costs one sequential pass over the file (page bodies are
 * skipped, not read into memory).
 */
class OggIndex {
public:
    void build(const std::shared_ptr<IInputStream> &stream);
    const std::vector<OggChainInfo> &chains() const { return m_chains; }
private:
    std::vector<OggChainInfo> m_chains;
};

#endif
