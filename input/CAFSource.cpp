#include "CAFSource.h"
#include "CoreAudioTypes.h"
#ifdef QAAC
#include "CoreAudioPacketDecoder.h"
#else
#include "ALACPacketDecoder.h"
#endif
#include "FLACPacketDecoder.h"
#include "LPCMPacketDecoder.h"
#include "cautil.h"
#include "chanmap.h"

CAFSource::CAFSource(std::shared_ptr<IInputStream> stream)
    : m_position(0)
    , m_position_raw(0)
    , m_currentPacket(0)
    , m_start_skip(0)
    , m_packetsPerChunk(1)
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
    m_decodeBuffer.set_unit(m_oasbd.mBytesPerFrame);
}

size_t CAFSource::readSamples(void *buffer, size_t nsamples)
{
    auto &&asbd = m_file->format().asbd;
    if (m_decodeBuffer.count() == 0) {
        fillDecodeBuffer();
    }
    if (nsamples > m_decodeBuffer.count())
        nsamples = m_decodeBuffer.count();
    if (nsamples > 0) {
        std::memcpy(buffer, m_decodeBuffer.read(nsamples), nsamples * m_oasbd.mBytesPerFrame);
        m_position += nsamples;
    }
    return nsamples;
}

void CAFSource::seekTo(int64_t count)
{
    if (count >= length()) return;
    auto &&asbd = m_file->format().asbd;
    m_position = count;
    m_decoder->reset();
    int64_t offsetInMediaTime = m_position + m_file->start_offset();
    m_currentPacket = std::max((offsetInMediaTime - getMaxFrameDependency() * asbd.mFramesPerPacket) / asbd.mFramesPerPacket, 0LL);
    int prerollSamples = offsetInMediaTime - m_currentPacket * asbd.mFramesPerPacket;
    while (prerollSamples > 0) {
        readPacket(& m_packetBuffer);
        size_t samples = m_decoder->decode(m_packetBuffer, &m_rawDecodeBuffer);
        if (samples > prerollSamples) {
            m_decodeBuffer.reserve(samples - prerollSamples);
            memcpy(m_decodeBuffer.write_ptr(), &m_rawDecodeBuffer[prerollSamples * m_oasbd.mBytesPerFrame], (samples - prerollSamples) * m_oasbd.mBytesPerFrame);
            m_decodeBuffer.commit(samples - prerollSamples);
        }
        prerollSamples -= samples;
    }
}

bool CAFSource::readPacket(std::vector<uint8_t> *buffer)
{
    if (m_currentPacket >= m_file->num_packets()) {
        buffer->resize(0);
        return false;
    }
    uint32_t n = m_file->read_packets(m_currentPacket, m_packetsPerChunk, buffer);
    m_currentPacket += n;
    return n > 0;
}

void CAFSource::fillDecodeBuffer()
{
    while (m_decodeBuffer.count() == 0) {
        bool ok = readPacket(&m_packetBuffer);
        int nsamples = m_decoder->decode(m_packetBuffer, &m_rawDecodeBuffer);
        if (m_position + m_decodeBuffer.count() + nsamples > m_file->duration()) {
            nsamples = std::max(0LL, m_file->duration() - m_position - int(m_decodeBuffer.count()));
        }
        if (!ok && nsamples == 0) break;
        if (nsamples > 0) {
            m_decodeBuffer.reserve(nsamples);
            std::memcpy(m_decodeBuffer.write_ptr(), m_rawDecodeBuffer.data(), m_rawDecodeBuffer.size());
            m_decodeBuffer.commit(nsamples);
        }
    }
}

void CAFSource::setupLPCM()
{
    auto &&asbd = m_file->format().asbd;
    m_decoder = std::make_shared<LPCMPacketDecoder>();
    std::vector<uint8_t> cookie(sizeof(AudioStreamBasicDescription));
    std::memcpy(cookie.data(), &asbd, sizeof(AudioStreamBasicDescription));
    m_decoder->setMagicCookie(cookie);
    m_oasbd = m_decoder->getSampleFormat();
    if (asbd.mBytesPerPacket > 0) {
        while (m_packetsPerChunk * asbd.mBytesPerPacket < 4096)
            m_packetsPerChunk <<= 1;
    }
}

void CAFSource::setupALAC()
{
    std::vector<std::uint8_t> cookie;
    m_file->get_magic_cookie(&cookie);
    auto &&asbd = m_file->format().asbd;

#ifdef QAAC
    m_decoder = std::make_shared<CoreAudioPacketDecoder>(asbd);
    m_decoder->setMagicCookie(cookie);
#else
    m_decoder = std::make_shared<ALACPacketDecoder>(asbd);
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
    m_decoder = std::make_shared<FLACPacketDecoder>();
    m_decoder->setMagicCookie(cookie);
    m_oasbd = m_decoder->getSampleFormat();
}

#ifdef QAAC
void CAFSource::setupMPEG1Audio()
{
    m_decoder = std::make_shared<CoreAudioPacketDecoder>(m_file->format().asbd);
    m_oasbd = m_decoder->getSampleFormat();
}

void CAFSource::setupMPEG4Audio()
{
    std::vector<std::uint8_t> cookie;
    m_file->get_magic_cookie(&cookie);
    m_decoder = std::make_shared<CoreAudioPacketDecoder>(m_file->format().asbd);
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
