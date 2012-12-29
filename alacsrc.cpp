#include <stdint.h>
#include <ALACBitUtilities.h>
#include "strutil.h"
#include "alacsrc.h"
#include "itunetags.h"
#include "cautil.h"
#include "chanmap.h"

ALACSource::ALACSource(const std::shared_ptr<FILE> &fp)
    : m_position(0), m_fp(fp)
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
        if (std::strcmp(type, "alac"))
            throw std::runtime_error("Not an ALAC file");

        const char *alacprop, *chanprop;
        const char *brand = m_file.GetStringProperty("ftyp.majorBrand");
        if (!std::strcmp(brand, "qt  ")) {
            // throw std::runtime_error("Not supported format");
            alacprop = "mdia.minf.stbl.stsd.alac.wave.alac.decoderConfig";
            chanprop = "mdia.minf.stbl.stsd.alac.wave.chan.data";
        } else {
            alacprop = "mdia.minf.stbl.stsd.alac.alac.decoderConfig";
            chanprop = "mdia.minf.stbl.stsd.alac.chan.data";
        }

        std::vector<uint8_t> alac, chan;
        uint8_t *value;
        uint32_t size;
        m_file.GetTrackBytesProperty(m_track_id, alacprop, &value, &size);
        std::copy(value + 4, value + size, std::back_inserter(alac));
        MP4Free(value);
        value = 0;
        try {
            m_file.GetTrackBytesProperty(m_track_id, chanprop, &value, &size);
            std::copy(value + 4, value + size, std::back_inserter(chan));
            MP4Free(value);
        } catch (...) {}
        if (alac.size() != 24 || (chan.size() && chan.size() < 12))
            throw std::runtime_error("ALACSource: invalid magic cookie");

        uint32_t timeScale;
        std::memcpy(&timeScale, &alac[20], 4);
        timeScale = util::b2host32(timeScale);
        m_asbd = cautil::buildASBDForPCM(timeScale, alac[9], alac[5],
                                    kAudioFormatFlagIsSignedInteger,
                                    kAudioFormatFlagIsAlignedHigh);
        m_oasbd = cautil::buildASBDForPCM2(timeScale, alac[9], alac[5],
                                           32, kAudioFormatFlagIsSignedInteger);

        m_buffer.units_per_packet = m_asbd.mBytesPerFrame;

        AudioChannelLayout acl = { 0 };
        if (chan.size()) {
            util::fourcc tag(reinterpret_cast<const char*>(&chan[0]));
            util::fourcc bitmap(reinterpret_cast<const char*>(&chan[4]));
            acl.mChannelLayoutTag = tag;
            acl.mChannelBitmap = bitmap;
            chanmap::getChannels(&acl, &m_chanmap);
        }
        m_decoder = std::shared_ptr<ALACDecoder>(new ALACDecoder());
        CHECKCA(m_decoder->Init(&alac[0], alac.size()));
        m_length = m_file.GetTrackDuration(m_track_id);

        mp4a::fetchTags(m_file, &m_tags);
        m_file.GetChapters(&m_chapters);
    } catch (mp4v2::impl::Exception *e) {
        handle_mp4error(e);
    }
}

size_t ALACSource::readSamples(void *buffer, size_t nsamples)
{
    uint32_t bpf = m_asbd.mBytesPerFrame;

    if (!m_buffer.count()) {
        uint32_t size;
        MP4SampleId sid;
        try {
            sid = m_file.GetSampleIdFromTime(m_track_id, m_position);
            size = m_file.GetSampleSize(m_track_id, sid);
        } catch (mp4v2::impl::Exception *e) {
            delete e;
            return 0;
        }
        MP4Timestamp start;
        MP4Duration duration;
        std::vector<uint8_t> ivec(size);
        uint8_t *vp = &ivec[0];

        try {
            m_file.ReadSample(m_track_id, sid, &vp, &size, &start,
                              &duration);
        } catch (mp4v2::impl::Exception *e) {
            handle_mp4error(e);
        }
        BitBuffer bits;
        BitBufferInit(&bits, vp, size);
        m_buffer.resize(duration);
        uint32_t ncount;
        CHECKCA(m_decoder->Decode(&bits, m_buffer.write_ptr(),
                                  duration, m_asbd.mChannelsPerFrame,
                                  &ncount));
        m_buffer.commit(ncount);
        m_buffer.advance(m_position - start);
    }
    uint32_t count = std::min(m_buffer.count(),
                              static_cast<uint32_t>(nsamples));
    size_t nbytes = count * bpf;
    util::unpack(m_buffer.read_ptr(), buffer, &nbytes,
                 m_asbd.mBytesPerFrame / m_asbd.mChannelsPerFrame,
                 m_oasbd.mBytesPerFrame / m_oasbd.mChannelsPerFrame);
    m_buffer.advance(count);
    m_position += count;
    return count;
}
