#include "MP4Source.h"
#include "MP4Edits.h"
#include "metadata.h"
#include "cautil.h"
#include "chanmap.h"
#ifdef QAAC
#include "CoreAudioPacketDecoder.h"
#include "MPAHeader.h"
#else
#include "ALACPacketDecoder.h"
#endif
#include "FLACPacketDecoder.h"
#include "OpusPacketDecoder.h"

MP4Source::MP4Source(std::shared_ptr<IInputStream> stream)
    : m_position(0),
      m_nextPacket(0),
      m_stream(stream),
      m_currentEdit(0),
      m_currentEditEndPosition(0)
{
    try {
        {
            util::FilePositionSaver _(m_stream);
            m_stream->seek(0, SEEK_SET);
            char buf[8];
            if (m_stream->read(buf, 8) != 8 || std::memcmp(&buf[4], "ftyp", 4))
                throw std::runtime_error("Not an MP4 file");
        }
        static MP4InputStreamCallbacks callbacks;
        m_file.Read(nullptr, nullptr, &callbacks, m_stream.get());
        m_track_id = m_file.FindTrackId(0, MP4_AUDIO_TRACK_TYPE);

        const char *type = m_file.GetTrackMediaDataName(m_track_id);
        if (!type)
            throw std::runtime_error("Multiple sample descriptions "
                                     "found in input");

        switch (util::fourcc(type).nvalue) {
        case 'alac': setupALAC();       break;
        case 'fLaC': setupFLAC();       break;
#ifdef QAAC
        case 'mp4a': setupMPEG4Audio(); break;
#endif
        case 'Opus': setupOpus();       break;
        default:     throw std::runtime_error("Not supported input codec");
        }

        m_decodeBuffer.set_unit(m_oasbd.mBytesPerFrame);
        m_tags = M4A::fetchTags(m_file);
        std::vector<MP4Edits::entry_t> edits;
        auto movieTimescale = m_file.GetTimeScale();
        auto mediaTimescale = m_file.GetTrackTimeScale(m_track_id);
        bool inaccurate_edit = movieTimescale < mediaTimescale;
        if (m_file.FindTrackAtom(m_track_id, "edts.elst")) {
            uint32_t nedits = m_file.GetTrackNumberOfEdits(m_track_id);
            if (inaccurate_edit && nedits == 1) {
                int64_t off = m_file.GetTrackEditMediaStart(m_track_id, 0);
                int64_t len = m_file.GetTrackDuration(m_track_id);
                m_edits.addEntry(off, len - off);
            } else {
                for (uint32_t i = 1; i <= nedits; ++i) {
                    int64_t off = m_file.GetTrackEditMediaStart(m_track_id, i);
                    double  len = m_file.GetTrackEditDuration(m_track_id, i);
                    m_edits.addEntry(off, len / movieTimescale * mediaTimescale + .5);
                }
            }
        }
        if (m_tags.find("iTunSMPB") != m_tags.end()) {
            if (m_edits.count() == 0) {
                std::string iTunSMPB = m_tags.find("iTunSMPB")->second;
                uint32_t junk, priming, padding;
                uint64_t duration;
                if (std::sscanf(iTunSMPB.c_str(), "%x %x %x %llx",
                    &junk, &priming, &padding, &duration) == 4) {
                    m_edits.addEntry(priming, duration);
                }
            }
        }
        bool is_nero = false;
        auto res = m_file.GetQTChapters(&m_chapters);
        double first_off = 0.0;
        if (!res) {
            if (m_file.GetNeroChapters(&m_chapters, &first_off))
                is_nero = true;
        }
        if (is_nero && !m_edits.count()) {
            /* assume old nero style handling of priming samples + padding */
            uint32_t priming = first_off * m_iasbd.mSampleRate + .5;
            uint64_t dur = m_file.GetTrackDuration(m_track_id);
            m_edits.addEntry(priming, dur - priming);
        }
        if (!m_edits.count())
            m_edits.addEntry(0, m_file.GetTrackDuration(m_track_id));

        uint32_t timescale = m_file.GetTrackTimeScale(m_track_id);
        if (m_oasbd.mSampleRate != timescale) {
            m_edits.scaleShift(m_oasbd.mSampleRate / timescale);
        }
        if (m_iasbd.mFormatID == 'aach' || m_iasbd.mFormatID == 'aacp') {
            /* 
             * When upsampled scale is used in HE-AAC/HE-AACv2 MP4.
             * we assume it is from Nero or FhG. We have to subtract
             * SBR decoder delay counted in ther number of priming samples.
             */
            if (m_oasbd.mSampleRate == timescale)
                m_edits.shiftMediaOffset(-481 * 2);
        }
    } catch (mp4v2::impl::Exception *e) {
        handle_mp4error(e);
    }
}

size_t MP4Source::readSamples(void *buffer, size_t nsamples)
{
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

void MP4Source::seekTo(int64_t count)
{
    if (count >= length()) {
        m_nextPacket = m_file.GetTrackNumberOfSamples(m_track_id);
        return;
    }
    m_decoder->reset();
    int64_t offsetInEdit;
    m_currentEdit = m_edits.editForPosition(count, &offsetInEdit);
    int64_t offsetInMediaTime = m_edits.mediaOffset(m_currentEdit) + offsetInEdit;
    m_position = count;
    int prerollSamplesInMediaTime = getMaxFrameDependency() * m_iasbd.mFramesPerPacket;
    if (offsetInMediaTime < prerollSamplesInMediaTime) {
        prerollSamplesInMediaTime = offsetInMediaTime;
    }
    m_nextPacket = (offsetInMediaTime - prerollSamplesInMediaTime) / m_iasbd.mFramesPerPacket;
    int prerollSamples = offsetInMediaTime - m_nextPacket * m_iasbd.mFramesPerPacket;
    while (prerollSamples > 0) {
        readPacket(&m_packetBuffer);
        size_t samples = m_decoder->decode(m_packetBuffer, &m_rawDecodeBuffer);
        if (samples > prerollSamples) {
            m_decodeBuffer.reserve(samples - prerollSamples);
            memcpy(m_decodeBuffer.write_ptr(), &m_rawDecodeBuffer[prerollSamples * m_oasbd.mBytesPerFrame], (samples - prerollSamples) * m_oasbd.mBytesPerFrame);
            m_decodeBuffer.commit(samples - prerollSamples);
        }
        prerollSamples -= samples;
    }
    m_currentEditEndPosition = m_edits.endPosition(m_currentEdit);
}

bool MP4Source::readPacket(std::vector<uint8_t> *buffer)
{
    if (m_nextPacket >= m_file.GetTrackNumberOfSamples(m_track_id)) {
        buffer->resize(0);
        return false;
    }
    uint32_t size = m_file.GetSampleSize(m_track_id, m_nextPacket + 1);
    buffer->resize(size);
    uint8_t *bp = buffer->data();
    MP4Timestamp dts;
    MP4Duration  duration;
    m_file.ReadSample(m_track_id, m_nextPacket + 1, &bp, &size, &dts,
                      &duration);
    ++m_nextPacket;
    return true;
}

void MP4Source::fillDecodeBuffer()
{
    while (m_decodeBuffer.count() == 0) {
        if (m_position + m_decodeBuffer.count() >= m_currentEditEndPosition)
            seekTo(m_position);
        bool ok = readPacket(&m_packetBuffer);
        int nsamples = m_decoder->decode(m_packetBuffer, &m_rawDecodeBuffer);
        if (m_position + m_decodeBuffer.count() + nsamples > m_currentEditEndPosition) {
            nsamples = std::max(0LL, m_currentEditEndPosition - m_position - int(m_decodeBuffer.count()));
        }
        if (!ok && nsamples == 0) break;
        if (nsamples > 0) {
            m_decodeBuffer.reserve(nsamples);
            std::memcpy(m_decodeBuffer.write_ptr(), m_rawDecodeBuffer.data(), m_rawDecodeBuffer.size());
            m_decodeBuffer.commit(nsamples);
        }
    }
}

void MP4Source::setupALAC()
{
    const char *brand = m_file.GetStringProperty("ftyp.majorBrand");
    const char *alacprop, *chanatom, *chanprop;
    if (!std::strcmp(brand, "qt  ")) {
        alacprop = "mdia.minf.stbl.stsd.alac.wave.alac.decoderConfig";
        chanatom = "mdia.minf.stbl.stsd.alac.wave.chan";
        chanprop = "mdia.minf.stbl.stsd.alac.wave.chan.data";
    } else {
        alacprop = "mdia.minf.stbl.stsd.alac.alac.decoderConfig";
        chanatom = "mdia.minf.stbl.stsd.alac.chan";
        chanprop = "mdia.minf.stbl.stsd.alac.chan.data";
    }
    std::vector<uint8_t> alac, chan;
    {
        uint8_t *value;
        uint32_t size;
        m_file.GetTrackBytesProperty(m_track_id, alacprop, &value, &size);
        std::copy(value + 4, value + size, std::back_inserter(alac));
        MP4Free(value);
    }
    {
        uint8_t *value;
        uint32_t size;
        if (m_file.FindTrackAtom(m_track_id, chanatom)) {
            m_file.GetTrackBytesProperty(m_track_id, chanprop, &value, &size);
            std::copy(value + 4, value + size, std::back_inserter(chan));
            MP4Free(value);
        }
    }
    if (alac.size() < 24)
        throw std::runtime_error("Malformed ALAC magic cookie");
    {
        uint32_t framelen, timescale;
        std::memcpy(&framelen,  &alac[0], 4);
        framelen  = util::b2host32(framelen);
        std::memcpy(&timescale, &alac[20], 4);
        timescale = util::b2host32(timescale);
        uint8_t  nchannels = alac[9];
        uint8_t  bits      = alac[5];
        memset(&m_iasbd, 0, sizeof m_iasbd);
        m_iasbd.mSampleRate = timescale;
        m_iasbd.mFormatID   = 'alac';
        switch (bits) {
        case 16: m_iasbd.mFormatFlags = 1; break;
        case 20: m_iasbd.mFormatFlags = 2; break;
        case 24: m_iasbd.mFormatFlags = 3; break;
        case 32: m_iasbd.mFormatFlags = 4; break;
        }
        m_iasbd.mFramesPerPacket  = framelen;
        m_iasbd.mChannelsPerFrame = nchannels;
    }
    if (chan.size() >= 12) {
        AudioChannelLayout acl = { 0 };
        util::fourcc tag(reinterpret_cast<const char*>(&chan[0]));
        util::fourcc bitmap(reinterpret_cast<const char*>(&chan[4]));
        acl.mChannelLayoutTag = tag;
        acl.mChannelBitmap    = bitmap;
        m_chanmap = chanmap::getChannels(&acl);
    } else if (m_iasbd.mChannelsPerFrame <= 8) {
        AudioChannelLayout acl = { 0 };
        acl.mChannelLayoutTag = chanmap::getALACChannelLayoutTag(m_iasbd.mChannelsPerFrame);
        m_chanmap = chanmap::getChannels(&acl);
	}
#ifdef QAAC
    m_decoder = std::make_shared<CoreAudioPacketDecoder>(m_iasbd);
#else
    m_decoder = std::make_shared<ALACPacketDecoder>(m_iasbd);
#endif
    m_decoder->setMagicCookie(alac);
    m_oasbd   = m_decoder->getSampleFormat();
}

void MP4Source::setupFLAC()
{
    const char *dfLaprop = "mdia.minf.stbl.stsd.fLaC.dfLa.data";
    std::vector<uint8_t> dfLa;
    {
        uint8_t *value;
        uint32_t size;
        m_file.GetTrackBytesProperty(m_track_id, dfLaprop, &value, &size);
        std::copy(value + 4, value + size, std::back_inserter(dfLa));
        MP4Free(value);
    }
    m_decoder =
        std::make_shared<FLACPacketDecoder>();
    m_decoder->setMagicCookie(dfLa);
    m_iasbd = std::dynamic_pointer_cast<FLACPacketDecoder>(m_decoder)->getInputFormat();
    m_oasbd = m_decoder->getSampleFormat();
}

#ifdef QAAC
void MP4Source::setupMPEG4Audio()
{
    uint8_t object_type = m_file.GetTrackEsdsObjectTypeId(m_track_id);
    /*
     * 0x40: MPEG-4
     * 0x67: MPEG-2 AAC
     * 0x69: MPEG-2 layer 1/2/3
     * 0x6b: MPEG-1 layer 1/2/3
     */
    switch (object_type) {
    case 0x40:
    case 0x67:
    {
        std::vector<uint8_t> magic_cookie;
        {
            mp4v2::impl::MP4Atom *esds =
                m_file.FindTrackAtom(m_track_id, "mdia.minf.stbl.stsd.*.esds");
            m_file.EnableMemoryBuffer();
            esds->Write();
            uint8_t *p = 0;
            uint64_t size;
            m_file.DisableMemoryBuffer(&p, &size);
            std::copy(p + 12, p + size, std::back_inserter(magic_cookie));
            MP4Free(p);
        }
        auto asc = cautil::parseMagicCookieAAC(magic_cookie);
        cautil::parseASC(asc, &m_iasbd, &m_chanmap);
        if (m_iasbd.mFormatID != 'aac ' &&
            m_iasbd.mFormatID != 'aach' &&
            m_iasbd.mFormatID != 'aacp')
            throw std::runtime_error("Not supported input codec");
        m_decoder = std::make_shared<CoreAudioPacketDecoder>(m_iasbd);
        m_decoder->setMagicCookie(magic_cookie);
        m_oasbd = m_decoder->getSampleFormat();
        break;
    }
    case 0x69:
    case 0x6b:
    {
        if (!m_file.GetTrackNumberOfSamples(m_track_id))
            throw std::runtime_error("Empty MPEG-1/2 audio input");
        std::vector<uint8_t> first_frame;
        readPacket(&first_frame);
        m_nextPacket = 0;
        MPAHeader header(first_frame.data());
        uint32_t layer_tab[] = { 0, '.mp3', '.mp2', '.mp1' };
        memset(&m_iasbd, 0, sizeof m_iasbd);
        m_iasbd.mSampleRate       = header.sample_rate();
        m_iasbd.mFormatID         = layer_tab[header.layer];
        m_iasbd.mFramesPerPacket  = header.samples_per_frame();
        m_iasbd.mChannelsPerFrame = header.is_mono() ? 1 : 2; 
        m_decoder = std::make_shared<CoreAudioPacketDecoder>(m_iasbd);
        m_oasbd = m_decoder->getSampleFormat();
        break;
    }
    default:
        throw std::runtime_error("Not supported input codec");
    }
}
#endif

void MP4Source::setupOpus()
{
    const char *dOpsprop = "mdia.minf.stbl.stsd.Opus.dOps.data";
    std::vector<uint8_t> dOps;
    {
        uint8_t *value;
        uint32_t size;
        m_file.GetTrackBytesProperty(m_track_id, dOpsprop, &value, &size);
        std::copy(value, value + size, std::back_inserter(dOps));
        MP4Free(value);
    }
    m_decoder = std::make_shared<OpusPacketDecoder>();
    m_decoder->setMagicCookie(dOps);
    readPacket(&m_packetBuffer);
    m_nextPacket = 0;
    m_decoder->decode(m_packetBuffer, &m_rawDecodeBuffer);
    m_iasbd = std::dynamic_pointer_cast<OpusPacketDecoder>(m_decoder)->getInputFormat();
    m_oasbd = m_decoder->getSampleFormat();
}

unsigned MP4Source::getMaxFrameDependency()
{
    switch (m_iasbd.mFormatID) {
    case 'alac':
    case 'fLaC':
        return 0;
    case '.mp3': return 10;
    case 'aach':
    case 'aacp':
        return m_iasbd.mSampleRate / 2 / m_iasbd.mFramesPerPacket;
    case 'opus':
        return 4;
    }
    return 1;
}

/* It looks like AudioConverter trims decoder delay by default */
unsigned MP4Source::getDecoderDelay()
{
    return 0;
}
