#include "MP4Source.h"
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

unsigned
MP4Edits::editForPosition(int64_t position, int64_t *offset_in_edit) const
{
    int64_t acc = 0;
    int64_t off = 0;
    size_t  i = 0;
    for (; i < m_edits.size(); ++i) {
        off =  position - acc;
        acc += m_edits[i].second;
        if (position < acc)
            break;
    }
    if (offset_in_edit) *offset_in_edit = off;
    return i == m_edits.size() ? i - 1 : i;
}

MP4Source::MP4Source(std::shared_ptr<IInputStream> stream)
    : m_position(0),
      m_position_raw(0),
      m_current_packet(0),
      m_stream(stream),
      m_time_ratio(1.0)
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

        m_decode_buffer.set_unit(m_oasbd.mBytesPerFrame);
        m_tags = M4A::fetchTags(m_file);
        if (m_file.FindTrackAtom(m_track_id, "edts.elst")) {
            uint32_t nedits = m_file.GetTrackNumberOfEdits(m_track_id);
            for (uint32_t i = 1; i <= nedits; ++i) {
                int64_t off = m_file.GetTrackEditMediaStart(m_track_id, i);
                double  len = m_file.GetTrackEditDuration(m_track_id, i);
                if (m_file.GetTimeScale() < m_file.GetTrackTimeScale(m_track_id)) {
                    // XXX: When the movie timescale is smaller than the media timescale,
                    // we cannot get sample accurate duration.
                    // In order to avoid trimming too much,
                    // increase duration by 1 here since value in the elst is likely rounded-down.
                    len += 1.0;
                }
                len /= m_file.GetTimeScale();
                len *= m_file.GetTrackTimeScale(m_track_id);
                if (m_file.GetTimeScale() < m_file.GetTrackTimeScale(m_track_id)) {
                    --len;
                }
                if (len <= 0.0 || len + off > m_file.GetTrackDuration(m_track_id))
                    len = m_file.GetTrackDuration(m_track_id) - off;

                m_edits.addEntry(off, len + .5);
            }
        }
        if (!m_edits.count() && m_tags.find("iTunSMPB") != m_tags.end()) {
            std::string iTunSMPB = m_tags.find("iTunSMPB")->second;
            uint32_t junk, priming, padding;
            uint64_t duration;
            if (std::sscanf(iTunSMPB.c_str(), "%x %x %x %llx",
                            &junk, &priming, &padding, &duration) == 4) {
                m_edits.addEntry(priming, duration);
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
            m_time_ratio = m_oasbd.mSampleRate / timescale;
            m_edits.scaleShift(m_time_ratio);
        }
        if (m_iasbd.mFormatID == 'aach' || m_iasbd.mFormatID == 'aacp') {
            /* 
             * When upsampled scale is used in HE-AAC/HE-AACv2 MP4.
             * we assume it is from Nero or FhG. We have to subtract
             * SBR decoder delay counted in ther number of priming samples.
             */
            if (m_time_ratio == 1.0)
                m_edits.shiftMediaOffset(-481 * 2);
        }
    } catch (mp4v2::impl::Exception *e) {
        handle_mp4error(e);
    }
}

size_t MP4Source::readSamples(void *buffer, size_t nsamples)
{
    int64_t off;
    unsigned edit = m_edits.editForPosition(m_position, &off);

    if (!m_decode_buffer.count()) {
        if (m_position > 0 && off == 0)
            seekTo(m_position);

        if (m_current_packet >= m_file.GetTrackNumberOfSamples(m_track_id))
            return 0;
        for (;;) {
            MP4Duration delta =
                m_file.GetSampleDuration(m_track_id, m_current_packet + 1);
            ssize_t nframes = static_cast<ssize_t>(delta * m_time_ratio + .5);
            m_decode_buffer.reserve(nframes);
            nframes = m_decoder->decode(m_decode_buffer.write_ptr(), nframes);
            m_position_raw += nframes;
            int64_t trim = std::max(m_position_raw
                                    - m_edits.mediaOffset(edit)
                                    - m_edits.duration(edit)
                                    - getDecoderDelay(),
                                    static_cast<int64_t>(0));
            if (trim > 0)
                nframes -= trim;
            if (nframes > 0)
                m_decode_buffer.commit(nframes);
            if (!m_decode_buffer.count())
                return 0;
            if (m_start_skip >= nframes) {
                m_decode_buffer.reset();
                m_start_skip -= nframes;
                continue;
            } else if (m_start_skip) {
                m_decode_buffer.advance(m_start_skip);
                nframes -= m_start_skip;
                m_start_skip = 0;
            }
            m_position += nframes;
            break;
        }
    }
    nsamples = std::min(m_decode_buffer.count(), nsamples);
    std::memcpy(buffer, m_decode_buffer.read(nsamples),
                nsamples * m_oasbd.mBytesPerFrame);
    return nsamples;
}

void MP4Source::seekTo(int64_t count)
{
    if (count >= length()) {
        m_position = length();
        m_current_packet = m_file.GetTrackNumberOfSamples(m_track_id);
        return;
    }
    m_decode_buffer.reset();
    m_decoder->reset();
    m_position = count;
    int64_t  mediapos  = m_edits.mediaOffsetForPosition(count);
    MP4Timestamp time  = static_cast<MP4Timestamp>(mediapos / m_time_ratio +.5);
    int64_t  ipacket   = m_file.GetSampleIdFromTime(m_track_id, time) - 1;
    time               = m_file.GetSampleTime(m_track_id, ipacket + 1);
    m_position_raw     = static_cast<int64_t>(time * m_time_ratio + .5);
    uint32_t preroll   = getMaxFrameDependency();
    m_current_packet   = std::max(0LL, ipacket - preroll);
    preroll            = ipacket - m_current_packet;
    m_start_skip       = mediapos - m_position_raw + getDecoderDelay();
    
    MP4Duration maxdelta = 0;
    for (uint32_t i = 0; i < preroll; ++i) {
        MP4Duration delta =
            m_file.GetSampleDuration(m_track_id, m_current_packet + i + 1);
        delta = static_cast<MP4Duration>(delta * m_time_ratio + .5);
        if (delta > maxdelta) maxdelta = delta;
    }
    std::vector<uint8_t> v(maxdelta * m_oasbd.mBytesPerFrame);
    for (uint32_t i = 0; i < preroll; ++i) {
        m_decoder->decode(v.data(), maxdelta);
    }
}

bool MP4Source::feed(std::vector<uint8_t> *buffer)
{
    if (m_current_packet >= m_file.GetTrackNumberOfSamples(m_track_id)) {
        buffer->resize(0);
        return false;
    }
    uint32_t size = m_file.GetSampleSize(m_track_id, m_current_packet + 1);
    buffer->resize(size);
    uint8_t *bp = buffer->data();
    MP4Timestamp dts;
    MP4Duration  duration;
    m_file.ReadSample(m_track_id, m_current_packet + 1, &bp, &size, &dts,
                      &duration);
    ++m_current_packet;
    return true;
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
    m_decoder = std::make_shared<CoreAudioPacketDecoder>(this, m_iasbd);
#else
    m_decoder = std::make_shared<ALACPacketDecoder>(this, m_iasbd);
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
        std::make_shared<FLACPacketDecoder>(this);
    m_decoder->setMagicCookie(dfLa);
    m_iasbd = ((FLACPacketDecoder*)m_decoder.get())->getInputFormat();
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
        m_decoder = std::make_shared<CoreAudioPacketDecoder>(this, m_iasbd);
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
        feed(&first_frame);
        m_current_packet = 0;
        MPAHeader header(first_frame.data());
        uint32_t layer_tab[] = { 0, '.mp3', '.mp2', '.mp1' };
        memset(&m_iasbd, 0, sizeof m_iasbd);
        m_iasbd.mSampleRate       = header.sample_rate();
        m_iasbd.mFormatID         = layer_tab[header.layer];
        m_iasbd.mFramesPerPacket  = header.samples_per_frame();
        m_iasbd.mChannelsPerFrame = header.is_mono() ? 1 : 2; 
        m_decoder = std::make_shared<CoreAudioPacketDecoder>(this, m_iasbd);
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
    m_decoder = std::make_shared<OpusPacketDecoder>(this);
    m_decoder->setMagicCookie(dOps);
    m_iasbd = ((FLACPacketDecoder*)m_decoder.get())->getInputFormat();
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
