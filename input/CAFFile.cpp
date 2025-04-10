#include <algorithm>
#include <iterator>
#define NOMINMAX
#include "CAFFile.h"
#include "cautil.h"
#include "chanmap.h"

namespace {
    template <typename InputIterator>
    unsigned read_ber_integer(InputIterator &begin, InputIterator end)
    {
        unsigned n = 0;
        uint8_t  b = 0x80;
        while (b >> 7 &&  begin != end) {
            b = *begin++;
            n <<= 7;
            n |=  b & 0x7f;
        }
        return n;
    }

    unsigned read_ber_integer(IInputStream *stream)
    {
        unsigned n = 0;
        uint8_t  b = 0x80;
        while (b >> 7) {
            if (stream->read(&b, 1) <= 0) break;
            n <<= 7;
            n |=  b & 0x7f;
        }
        return n;
    }
}

void CAFFile::get_magic_cookie(std::vector<uint8_t> *data) const
{
    data->resize(m_magic_cookie.size());
    std::copy(m_magic_cookie.begin(), m_magic_cookie.end(), data->begin());
}

int64_t CAFFile::packet_info(int64_t index, uint32_t *size) const
{
    int64_t  offset;

    if (index >= num_packets())
        throw std::runtime_error("invalid packet offset");

    if (m_packet_table.size() == 0) {
        auto asbd = format().asbd;
        offset = index * asbd.mBytesPerPacket;
        if (size) *size = asbd.mBytesPerPacket;
    } else {
        offset = m_packet_table[index].mStartOffset;
        if (size) *size = m_packet_table[index].mDataByteSize;
    }
    return offset;
}

uint32_t CAFFile::read_packets(int64_t offset, uint32_t count, std::vector<uint8_t> *data)
{
    count = std::max(std::min(offset + count, num_packets()) - offset,
                     static_cast<int64_t>(0));
    if (count == 0)
        data->resize(0);
    else {
        uint32_t size;
        int64_t bytes_offset = packet_info(offset, &size);
        uint32_t size_total = size;
        if (m_packet_table.size() == 0)
            size_total *= count;
        else {
            for (uint32_t i = 1; i < count; ++i)
                size_total += m_packet_table[offset + i].mDataByteSize;
        }
        data->resize(size_total);
        if (m_stream->seek(m_data_offset + bytes_offset, SEEK_SET) < 0) {
            data->resize(0);
            return 0;
        }
        int n = m_stream->read(data->data(), data->size());
        data->resize(n);
        count = count * n / size_total;
    }
    return count;
}

void CAFFile::read16be(void *n)
{
    uint16_t *p = static_cast<uint16_t*>(n);
    util::check_eof(m_stream->read(n, 2) == 2);
    *p = util::b2host16(*p);
}

void CAFFile::read32be(void *n)
{
    uint32_t *p = static_cast<uint32_t*>(n);
    util::check_eof(m_stream->read(n, 4) == 4);
    *p = util::b2host32(*p);
}

void CAFFile::read64be(void *n)
{
    uint64_t *p = static_cast<uint64_t*>(n);
    util::check_eof(m_stream->read(n, 8) == 8);
    *p = util::b2host64(*p);
}

uint32_t CAFFile::nextChunk(uint64_t *size)
{
    uint32_t fcc;
    uint64_t n;
    read32be(&fcc);
    read64be(&n);
    if (size) *size = n;
    return fcc;
}

void CAFFile::parse()
{
    uint32_t fcc;
    uint64_t  size;

    read32be(&fcc);
    if (fcc != FOURCC('c','a','f','f'))
        throw std::runtime_error("not a caf file");
    CHECKCRT(m_stream->seek(4, SEEK_CUR) < 0);

    for (;;) {
        int64_t pos = m_stream->tell();
        if (m_stream->seekable()) {
            int64_t remaining = m_stream->size() - pos;
            if (remaining < 12) {
                break;
            }
        }
        fcc = nextChunk(&size);

        switch (fcc) {
        case FOURCC('d','e','s','c'):
            parse_desc(&m_primary_format); break;
        case FOURCC('c','h','a','n'):
            parse_chan(&m_primary_format, size); break;
        case FOURCC('l','d','s','c'):
            parse_ldsc(size); break;
        case FOURCC('k','u','k','i'):
            parse_kuki(size); break;
        case FOURCC('i','n','f','o'):
            parse_info(size); break;
        case FOURCC('p','a','k','t'):
            parse_pakt(size); break;
        }
        if (fcc == FOURCC('d','a','t','a')) {
            m_data_offset = pos + 16;
            if (size == ~0uLL) {
                m_data_size = m_stream->seekable() ? m_stream->size() - m_data_offset : -1;
            } else {
                m_data_size = size - 4;
            }
            if (size == ~0uLL || !m_stream->seekable()) {
                break;
            }
        }
        m_stream->seek(pos + 12 + size, SEEK_SET);
    }
    if (m_primary_format.asbd.mFormatID == 0)
        throw std::runtime_error("desc chunk not found");
    if (m_data_offset == 0)
        throw std::runtime_error("data chunk not found");
    if (m_primary_format.asbd.mBytesPerPacket == 0 && m_packet_table.empty()) {
        throw std::runtime_error("pakt chunk not found");
    }
    calc_duration();
}

void CAFFile::parse_desc(Format *d)
{
    read64be(&d->asbd.mSampleRate);
    read32be(&d->asbd.mFormatID);
    read32be(&d->asbd.mFormatFlags);
    read32be(&d->asbd.mBytesPerPacket);
    read32be(&d->asbd.mFramesPerPacket);
    read32be(&d->asbd.mChannelsPerFrame);
    read32be(&d->asbd.mBitsPerChannel);
    d->asbd.mBytesPerFrame = d->asbd.mBytesPerPacket / d->asbd.mFramesPerPacket;
}

void CAFFile::parse_chan(Format *d, int64_t size)
{
    if (size < 12) {
        throw std::runtime_error("invalid chan chunk");
    }
    auto acl = std::shared_ptr<AudioChannelLayout>(static_cast<AudioChannelLayout*>(std::malloc(size)), std::free);
    read32be(&acl->mChannelLayoutTag);
    read32be(&acl->mChannelBitmap);
    read32be(&acl->mNumberChannelDescriptions);
    if (size - 12 < acl->mNumberChannelDescriptions * sizeof(AudioChannelDescription)) {
        throw std::runtime_error("invalid chan chunk");
    }
    for (unsigned i = 0; i < acl->mNumberChannelDescriptions; ++i) {
        read32be(&acl->mChannelDescriptions[i].mChannelLabel);
        read32be(&acl->mChannelDescriptions[i].mChannelFlags);
        read32be(&acl->mChannelDescriptions[i].mCoordinates[0]);
        read32be(&acl->mChannelDescriptions[i].mCoordinates[1]);
        read32be(&acl->mChannelDescriptions[i].mCoordinates[2]);
    }
    d->channel_layout = chanmap::getChannels(acl.get());
}

void CAFFile::parse_ldsc(int64_t size)
{
    int64_t pos = m_stream->tell();
    int64_t end = pos + size;
    while (m_stream->seek(0, SEEK_CUR) < end) {
        Format d;
        parse_desc(&d);
        AudioChannelLayout acl = { 0 };
        read32be(&acl.mChannelLayoutTag);
        d.channel_layout = chanmap::getChannels(&acl);
        m_layered_formats.push_back(d);
    }
}

void CAFFile::parse_kuki(int64_t size)
{
    m_magic_cookie.resize(size);
    m_stream->read(m_magic_cookie.data(), size);
}

void CAFFile::parse_info(int64_t size)
{
    if (size <= 4 || size > 1024 * 1024 * 64) {
        CHECKCRT(m_stream->seek(size, SEEK_CUR) < 0);
        return;
    }
    uint32_t num_info;
    std::vector<char> buf(size - 4);
    char *key, *val, *end;

    read32be(&num_info);
    m_stream->read(buf.data(), size - 4);

    key = buf.data();
    end = buf.data() + buf.size();
    do {
        if ((val = key + strlen(key) + 1) < end) {
            m_tags.push_back(std::make_pair(std::string(key),
                                                std::string(val)));
            key = val + strlen(val) + 1;
        }
    } while (key < end && val < end);
}

void CAFFile::parse_pakt(int64_t size)
{
    // CAFPacketTableHeader
    int64_t i, mNumberPackets, pos = 0;
    AudioStreamBasicDescription asbd = format().asbd;

    read64be(&mNumberPackets);
    read64be(&m_packet_info.mNumberValidFrames);
    read32be(&m_packet_info.mPrimingFrames);
    read32be(&m_packet_info.mRemainderFrames);

    uint32_t low = ~0, high = 0;

    for (i = 0; i < mNumberPackets; ++i) {
        AudioStreamPacketDescription aspd = { 0 };
        aspd.mStartOffset  = pos;
        if (!asbd.mBytesPerPacket)
            aspd.mDataByteSize = read_ber_integer(m_stream.get());
        else
            aspd.mDataByteSize = asbd.mBytesPerPacket;
        pos += aspd.mDataByteSize;
        if (low  > aspd.mDataByteSize) low  = aspd.mDataByteSize;
        if (high < aspd.mDataByteSize) high = aspd.mDataByteSize;

        if (!asbd.mFramesPerPacket)
            aspd.mVariableFramesInPacket = read_ber_integer(m_stream.get());
        else
            aspd.mVariableFramesInPacket = asbd.mFramesPerPacket;
        m_packet_table.push_back(aspd);
    }
}

void CAFFile::calc_duration()
{
    AudioStreamBasicDescription asbd = format().asbd;
    if (m_packet_info.mNumberValidFrames)
        m_duration = m_packet_info.mNumberValidFrames * tscale() + .5;
    else if (m_packet_table.size())
        m_duration = m_packet_table.size() * asbd.mFramesPerPacket;
    else if (asbd.mFramesPerPacket)
        m_duration = (m_data_size == -1) ? -1 : m_data_size / asbd.mBytesPerPacket * asbd.mFramesPerPacket;
    else
        throw std::runtime_error("variable frame length is not supported");
}
