#include "CAFSource.h"
#include "CoreAudioTypes.h"
#ifdef QAAC
#include "CoreAudioPacketDecoder.h"
#else
#include "ALACPacketDecoder.h"
#endif
#include "FLACPacketDecoder.h"
#include "cautil.h"
#include "chanmap.h"

CAFSource::CAFSource(std::shared_ptr<IInputStream> stream)
    : m_position(0)
    , m_position_raw(0)
    , m_current_packet(0)
    , m_start_skip(0)
    , m_packets_per_chunk(1)
{
    m_file = std::make_shared<CAFFile>(stream);
    for (auto &&e: m_file->get_tags()) {
        m_tags[e.first] = e.second;
    }
    m_chanmap = m_file->channel_layout();
    switch (m_file->format().asbd.mFormatID) {
        case kAudioFormatLinearPCM: setupLPCM();       break;
        case kAudioFormatAppleLossless: setupALAC();       break;
        case 'flac': setupFLAC();       break;
#ifdef QAAC
        case kAudioFormatMPEGLayer3: setupMPEG1Audio(); break;
        case kAudioFormatMPEG4AAC: setupMPEG4Audio(); break;
        case kAudioFormatMPEG4AAC_HE: setupMPEG4Audio(); break;
        case kAudioFormatMPEG4AAC_HE_V2: setupMPEG4Audio(); break;
#endif
        default:     throw std::runtime_error("Not supported input codec");
    }
    m_decode_buffer.set_unit(m_oasbd.mBytesPerFrame);
}

size_t CAFSource::readSamples(void *buffer, size_t nsamples)
{
    auto &&asbd = m_file->format().asbd;
    if (asbd.mFormatID == kAudioFormatLinearPCM)
        return readSamplesLPCM(buffer, nsamples);
    auto fpp = asbd.mFramesPerPacket;
    if (m_decode_buffer.count() == 0) {
        m_decode_buffer.reserve(fpp);
        int64_t n = m_decoder->decode(m_decode_buffer.write_ptr(), fpp);
        m_decode_buffer.commit(std::min(n, m_file->duration() - m_position));
        if (m_start_skip) {
            if (m_start_skip >= m_decode_buffer.count()) {
                m_start_skip -= m_decode_buffer.count();
                m_decode_buffer.reset();
                return readSamples(buffer, nsamples);
            }
            m_decode_buffer.advance(m_start_skip);
            m_start_skip = 0;
        }
    }
    if (m_decode_buffer.count() == 0) {
        return 0;
    }
    auto n = std::min(nsamples, m_decode_buffer.count());
    std::memcpy(buffer, m_decode_buffer.read_ptr(), n * m_oasbd.mBytesPerFrame);
    m_decode_buffer.advance(n);
    m_position += n;
    return n;
}

size_t CAFSource::readSamplesLPCM(void *buffer, size_t nsamples)
{
    auto &&asbd = m_file->format().asbd;
    if (m_decode_buffer.count() == 0) {
        int n = m_file->read_packets(m_current_packet, m_packets_per_chunk, &m_packet_buffer);
        if (n == 0) return 0;
        m_current_packet += n;
        size_t size = m_packet_buffer.size();
        if ((asbd.mFormatFlags & 2) == 0) {
            util::bswapbuffer(m_packet_buffer.data(), size, (asbd.mBitsPerChannel + 7) & ~7);
        }
        m_decode_buffer.reserve(size / asbd.mBytesPerFrame);
        util::unpack(m_packet_buffer.data(), m_decode_buffer.write_ptr(), &size,
                     asbd.mBytesPerFrame / asbd.mChannelsPerFrame,
                     m_oasbd.mBytesPerFrame / m_oasbd.mChannelsPerFrame);
        m_decode_buffer.commit(size / m_oasbd.mBytesPerFrame);
    }
    if (m_decode_buffer.count() == 0) return 0;
    if (m_decode_buffer.count() < nsamples) {
        nsamples = m_decode_buffer.count();
    }
    std::memcpy(buffer, m_decode_buffer.read_ptr(), nsamples * m_oasbd.mBytesPerFrame);
    m_decode_buffer.advance(nsamples);
    m_position += nsamples;
    return nsamples;
}

void CAFSource::seekTo(int64_t count)
{
    auto &&asbd = m_file->format().asbd;
    m_decode_buffer.reset();
    auto fpp = asbd.mFramesPerPacket;
    auto ipacket = count / fpp;
    if (m_decoder) {
        m_decoder->reset();
        auto ppacket = std::max(0LL, ipacket - getMaxFrameDependency());
        m_start_skip = count + m_file->start_offset() + getDecoderDelay() - ipacket * fpp;
        std::vector<std::uint8_t> tmp(asbd.mFramesPerPacket * asbd.mBytesPerFrame);
        while (ppacket < ipacket) {
            m_file->read_packets(ppacket++, 1, &m_packet_buffer);
            m_decoder->decode(tmp.data(), fpp);
        }
    }
    m_current_packet = ipacket;
    m_position = count;
}

bool CAFSource::feed(std::vector<uint8_t> *buffer)
{
    uint32_t n = m_file->read_packets(m_current_packet, m_packets_per_chunk, buffer);
    m_current_packet += n;
    return n > 0;
}

void CAFSource::setupLPCM()
{
    auto &&asbd = m_file->format().asbd;
    if (asbd.mFormatFlags & kAudioFormatFlagIsFloat) {
        m_oasbd = cautil::buildASBDForPCM2(asbd.mSampleRate, asbd.mChannelsPerFrame,
            asbd.mBitsPerChannel, asbd.mBitsPerChannel,
            kAudioFormatFlagIsFloat);
    } else {
        m_oasbd = cautil::buildASBDForPCM2(asbd.mSampleRate, asbd.mChannelsPerFrame,
            asbd.mBitsPerChannel, 32,
            kAudioFormatFlagIsSignedInteger);
    }

    if (asbd.mBytesPerPacket > 0) {
        while (m_packets_per_chunk * asbd.mBytesPerPacket < 4096)
            m_packets_per_chunk <<= 1;
    }
}

void CAFSource::setupALAC()
{
    std::vector<std::uint8_t> cookie;
    m_file->get_magic_cookie(&cookie);
    auto &&asbd = m_file->format().asbd;

#ifdef QAAC
    m_decoder = std::make_shared<CoreAudioPacketDecoder>(this, asbd);
    m_decoder->setMagicCookie(cookie);
#else
    m_decoder = std::make_shared<ALACPacketDecoder>(this, asbd);
    m_decoder->setMagicCookie(cookie);
#endif
    m_oasbd = m_decoder->getSampleFormat();
    if (m_chanmap.empty()) {
        AudioChannelLayout acl = { 0 };
        acl.mChannelLayoutTag = chanmap::getALACChannelLayoutTag(asbd.mChannelsPerFrame);
        m_chanmap = chanmap::getChannels(&acl);
    }
}

void CAFSource::setupFLAC()
{
    std::vector<std::uint8_t> cookie;
    m_file->get_magic_cookie(&cookie);
    if (cookie.size() > 12) {
        cookie.erase(cookie.begin(), cookie.begin() + 12);
    }
    m_decoder = std::make_shared<FLACPacketDecoder>(this);
    m_decoder->setMagicCookie(cookie);
    m_oasbd = m_decoder->getSampleFormat();
}

#ifdef QAAC
void CAFSource::setupMPEG1Audio()
{
    m_decoder = std::make_shared<CoreAudioPacketDecoder>(this, m_file->format().asbd);
    m_oasbd = m_decoder->getSampleFormat();
}

void CAFSource::setupMPEG4Audio()
{
    std::vector<std::uint8_t> cookie;
    m_file->get_magic_cookie(&cookie);
    m_decoder = std::make_shared<CoreAudioPacketDecoder>(this, m_file->format().asbd);
    m_decoder->setMagicCookie(cookie);
    m_oasbd = m_decoder->getSampleFormat();
    if (m_chanmap.empty()) {
        auto asc = cautil::parseMagicCookieAAC(cookie);
        AudioStreamBasicDescription tmp;
        cautil::parseASC(asc, &tmp, &m_chanmap);
    }
}
#endif

unsigned CAFSource::getMaxFrameDependency()
{
    auto &&asbd = m_file->format().asbd;
    switch (asbd.mFormatID) {
    case kAudioFormatLinearPCM:
    case kAudioFormatAppleLossless:
    case 'flac':
        return 0;
    case kAudioFormatMPEGLayer3: return 10;
    case kAudioFormatMPEG4AAC_HE:
    case kAudioFormatMPEG4AAC_HE_V2:
        return asbd.mSampleRate / 2 / asbd.mFramesPerPacket;
    }
    return 1;
}

/* It looks like AudioConverter trims decoder delay by default */
unsigned CAFSource::getDecoderDelay()
{
    return 0;
}
