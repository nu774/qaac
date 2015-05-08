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

MP4Source::MP4Source(const std::shared_ptr<FILE> &fp)
    : m_position(0),
      m_position_raw(0),
      m_current_packet(0),
      m_fp(fp)
{
    try {
        int fd = fileno(m_fp.get());
        {
            util::FilePositionSaver _(fd);
            _lseeki64(fd, 0, SEEK_SET);
            char buf[8];
            if (read(fd, buf, 8) != 8 || std::memcmp(&buf[4], "ftyp", 4))
                throw std::runtime_error("Not an MP4 file");
        }
        static MP4FDReadProvider provider;
        std::string name = strutil::format("%d", fd);
        m_file.Read(name.c_str(), &provider);
        m_track_id = m_file.FindTrackId(0, MP4_AUDIO_TRACK_TYPE);

        const char *type = m_file.GetTrackMediaDataName(m_track_id);
        if (!type)
            throw std::runtime_error("Multiple sample descriptions "
                                     "found in input");

        switch (util::fourcc(type).nvalue) {
        case 'alac': setupALAC();       break;
#ifdef QAAC
        case 'mp4a': setupMPEG4Audio(); break;
#endif
        default:     throw std::runtime_error("Not supported input codec");
        }

        m_decode_buffer.set_unit(m_oasbd.mBytesPerFrame);
        M4A::fetchTags(m_file, &m_tags);
        m_file.GetChapters(&m_chapters);

        if (m_file.GetTimeScale() >= m_file.GetTrackTimeScale(m_track_id)) {
            if (m_file.FindTrackAtom(m_track_id, "edts.elst")) {
                uint32_t nedits = m_file.GetTrackNumberOfEdits(m_track_id);
                for (uint32_t i = 1; i <= nedits; ++i) {
                    int64_t off = m_file.GetTrackEditMediaStart(m_track_id, i);
                    double  len = m_file.GetTrackEditDuration(m_track_id, i);
                    len /= m_file.GetTimeScale();
                    len *= m_file.GetTrackTimeScale(m_track_id);
                    if (len == 0.0)
                        len = m_file.GetTrackDuration(m_track_id) - off;
                    m_edits.addEntry(off, len + .5);
                }
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
        if (!m_edits.count())
            m_edits.addEntry(0, m_file.GetTrackDuration(m_track_id));

    } catch (mp4v2::impl::Exception *e) {
        handle_mp4error(e);
    }
}

size_t MP4Source::readSamples(void *buffer, size_t nsamples)
{
    unsigned fpp = m_iasbd.mFramesPerPacket;
    unsigned edit = m_edits.editForPosition(m_position, 0);
    m_decode_buffer.reserve(fpp);
    ssize_t nframes = m_decoder->decode(m_decode_buffer.write_ptr(),
                                        fpp);
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
    if (m_start_skip) {
        if (m_start_skip >= nframes) {
            m_start_skip -= nframes;
            return readSamples(buffer, nsamples);
        }
        m_decode_buffer.advance(m_start_skip);
        nframes -= m_start_skip;
        m_start_skip = 0;
    }
    m_position += nframes;
    nsamples = std::min(m_decode_buffer.count(), nsamples);
    std::memcpy(buffer, m_decode_buffer.read(nsamples),
                nsamples * m_oasbd.mBytesPerFrame);
    if (m_edits.editForPosition(m_position, 0) != edit)
        seekTo(m_position);
    return nsamples;
}

void MP4Source::seekTo(int64_t count)
{
    if (count >= length()) {
        m_position = length();
        m_current_packet = m_file.GetTrackNumberOfSamples(m_track_id) + 1;
        return;
    }
    m_decoder->reset();
    m_decode_buffer.reset();
    m_position = count;
    int64_t  mediapos  = m_edits.mediaOffsetForPosition(count);
    uint32_t fpp       = m_iasbd.mFramesPerPacket;
    int64_t  ipacket   = mediapos / fpp;
    uint32_t preroll   = getMaxFrameDependency();
    m_position_raw     = ipacket * fpp;
    m_current_packet   = std::max(0LL, ipacket - preroll);
    preroll            = ipacket - m_current_packet;
    m_start_skip = mediapos - ipacket * fpp + getDecoderDelay();
    
    std::vector<uint8_t> v(m_iasbd.mFramesPerPacket * m_oasbd.mBytesPerFrame);
    for (uint32_t i = 0; i < preroll; ++i)
        m_decoder->decode(v.data(), m_iasbd.mFramesPerPacket);
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
        chanmap::getChannels(&acl, &m_chanmap);
    }
#ifdef QAAC
    m_decoder = std::make_shared<CoreAudioPacketDecoder>(this, m_iasbd);
#else
    m_decoder = std::make_shared<ALACPacketDecoder>(this, m_iasbd);
#endif
    m_decoder->setMagicCookie(alac);
    m_oasbd   = m_decoder->getSampleFormat();
}

#ifdef QAAC
void MP4Source::setupMPEG4Audio()
{
    uint8_t object_type = m_file.GetTrackEsdsObjectTypeId(m_track_id);
    uint32_t codec = 0;
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
        std::vector<uint8_t> magic_cookie, asc;
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
        cautil::parseMagicCookieAAC(magic_cookie, &asc);
        cautil::parseASC(asc, &m_iasbd, &m_chanmap);
        if (m_iasbd.mFormatID != 'aac ')
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

unsigned MP4Source::getMaxFrameDependency()
{
    switch (m_iasbd.mFormatID) {
    case 'alac': return 0;
    case '.mp3': return 10;
    }
    return 1;
}

/* It looks like AudioConverter trims decoder delay by default */
unsigned MP4Source::getDecoderDelay()
{
    return 0;
}
