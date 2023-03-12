#include <algorithm>
#include "strutil.h"
#include "sink.h"
#include "util.h"
#include "bitstream.h"
#include "metadata.h"
#if defined(_MSC_VER) || defined(__MINGW32__)
#include "win32util.h"
#include <io.h>
#include <fcntl.h>
#endif
#include <sys/stat.h>

using mp4v2::impl::itmf::enumGenreType;
using mp4v2::impl::itmf::enumStikType;
using mp4v2::impl::itmf::enumAccountType;
using mp4v2::impl::itmf::enumCountryCode;
using mp4v2::impl::itmf::enumContentRating;
using mp4v2::impl::MP4Track;

static
size_t parseDecSpecificConfig(const std::vector<uint8_t> &config,
                              unsigned *sampling_rate_index,
                              unsigned *sampling_rate, unsigned *channel_config)
{
    static const unsigned tab[] = {
        96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050, 
        16000, 12000, 11025, 8000, 7350, 0, 0, 0
    };
    BitStream bs(const_cast<uint8_t*>(&config[0]), config.size());
    bs.get(5); // objtype
    *sampling_rate_index = bs.get(4);
    if (*sampling_rate_index == 15)
        *sampling_rate = bs.get(24);
    else
        *sampling_rate = tab[*sampling_rate_index];
    *channel_config = bs.get(4);
    return bs.position();
}

static
void parseMagicCookieALAC(const std::vector<uint8_t> &cookie,
                          std::vector<uint8_t> *alac,
                          std::vector<uint8_t> *chan)
{
    const uint8_t *cp = &cookie[0];
    const uint8_t *endp = cp + cookie.size();
    if (std::memcmp(cp + 4, "frmaalac", 8) == 0)
        cp += 24;
    if (endp - cp >= 24) {
        alac->resize(24);
        std::memcpy(&(*alac)[0], cp, 24);
        cp += 24;
        if (endp - cp >= 24 && !std::memcmp(cp + 4, "chan", 4)) {
            chan->resize(12);
            std::memcpy(&(*chan)[0], cp + 12, 12);
        }
    }
}

using mp4v2::impl::MP4Atom;

MP4SinkBase::MP4SinkBase(const std::wstring &path, bool temp)
        : m_filename(path), m_closed(false),
          m_edit_start(0), m_edit_duration(0),
          m_max_bitrate(0)
{
    static const char * const compatibleBrands[] =
        { "M4A ", "mp42", "isom", "" };
    if (temp) m_filename = L"qaac.int";
    try {
        static MP4StdIOCallbacks callbacks;
        if (path == L"-") {
            m_fp = std::shared_ptr<FILE>(stdout, [](FILE *){});
        } else if (temp) {
            m_fp = std::shared_ptr<FILE>(win32::tmpfile(m_filename.c_str()), fclose);
        } else {
            m_fp = std::shared_ptr<FILE>(win32::wfopenx(m_filename.c_str(), L"wb"), fclose);
        }

        m_mp4file.Create(strutil::w2us(m_filename).c_str(),
                         &callbacks,
                         m_fp.get(),
                         0, // flags
                         1, // add_ftypes
                         0, // add_iods
                         const_cast<char*>("M4A "), // majorBrand
                         0, // minorVersion
                         const_cast<char**>(compatibleBrands),
                         util::sizeof_array(compatibleBrands));
    } catch (mp4v2::impl::Exception *e) {
        m_mp4file.ResetFile();
        handle_mp4error(e);
    }
}

void MP4SinkBase::writeTags()
{
    std::map<uint32_t, std::string> shortTags;
    std::map<std::string, std::string> longTags;
    std::map<uint32_t, std::string>::const_iterator si;
    std::map<std::string, std::string>::const_iterator li;

    try {
        if (m_chapters.size()) {
            uint64_t timeScale =
                m_mp4file.GetIntegerProperty("moov.mvhd.timeScale");
            MP4TrackId track = m_mp4file.AddChapterTextTrack(1);
            /*
             * Historically, Nero AAC encoder was using chapter marker to
             * signal encoder delay, and fb2k seems to be in honor of it.
             * Therefore we delay the first chapter position of 
             * Nero style chapter.
             *
             * QuickTime chapter is duration based, therefore first chapter
             * always starts at beginning of the track, but last chapter can
             * end at arbitrary point.
             *
             * On the other hand, Nero chapter is offset(start time) based,
             * therefore first chapter can start at arbitrary point (and 
             * this is used to signal encoder delay).
             * However, last chapter always ends at track end.
             */
            double off = static_cast<double>(m_edit_start) / timeScale;
            std::vector<misc::chapter_t>::const_iterator chap;
            for (chap = m_chapters.begin(); chap != m_chapters.end(); ++chap) {
                std::string name = strutil::w2us(chap->first);
                const char *namep = name.c_str();
                m_mp4file.AddChapter(track, chap->second * timeScale + 0.5,
                                     namep);
                int64_t stamp = off * 10000000.0 + 0.5;
                m_mp4file.AddNeroChapter(stamp, namep);
                off += chap->second;
            }
        }

        M4A::convertToM4ATags(m_tags, &shortTags, &longTags);
        for (si = shortTags.begin(); si != shortTags.end(); ++si) {
            if (!si->second.size()) continue;
            if (si->first != Tag::kArtwork)
                writeShortTag(si->first, si->second);
        }
        for (li = longTags.begin(); li != longTags.end(); ++li) {
            if (li->second.size())
                writeLongTag(li->first, li->second);
        }
        for (size_t i = 0; i < m_artworks.size(); ++i)
            m_mp4file.SetMetadataArtwork("covr", &m_artworks[i][0],
                                         m_artworks[i].size());
    } catch (mp4v2::impl::Exception *e) {
        handle_mp4error(e);
    }
}

void MP4SinkBase::close()
{
    if (!m_closed) {
        m_closed = true;
        try {
            m_mp4file.Close(MP4_CLOSE_DO_NOT_COMPUTE_BITRATE);
        } catch (mp4v2::impl::Exception *e) {
            handle_mp4error(e);
        }
    }
}

void MP4SinkBase::updateMaxBitrate(bool finalize)
{
    MP4Track *track = m_mp4file.GetTrack(m_track_id);
    unsigned numSamples = track->GetNumberOfSamples();
    unsigned timeScale = track->GetTimeScale();
    unsigned duration = 0;
    unsigned size = 0;
    for (int sid = numSamples; sid >= 1 && duration < timeScale; --sid) {
        MP4Timestamp dts;
        MP4Duration delta;
        track->GetSampleTimes(sid, &dts, &delta);
        duration += delta;
        size += track->GetSampleSize(sid);
    }
    if (!duration || (!finalize && duration < timeScale))
        return;
    unsigned bitrate = size * 8.0 * timeScale / duration + .5;
    if (bitrate > m_max_bitrate)
        m_max_bitrate = bitrate;
}

void MP4SinkBase::writeBitrates(int avgBitrate)
{
    updateMaxBitrate(true);
    if (m_mp4file.FindTrackAtom(m_track_id, "mdia.minf.stbl.stsd.*.esds")) {
        m_mp4file.SetTrackIntegerProperty(m_track_id,
                                          "mdia.minf.stbl.stsd.*.esds.decConfigDescr.maxBitrate",
                                          m_max_bitrate);
        m_mp4file.SetTrackIntegerProperty(m_track_id,
                                          "mdia.minf.stbl.stsd.*.esds.decConfigDescr.avgBitrate",
                                          avgBitrate);
    }
}

void MP4SinkBase::writeShortTag(uint32_t fcc, const std::string &value)
{
    struct handler_t {
        uint32_t fcc;
        void (MP4SinkBase::*mf)(const char *, const std::string &);
    } handlers[] = {
        { Tag::kAlbum,                &MP4SinkBase::writeStringTag      },
        { Tag::kAlbumArtist,          &MP4SinkBase::writeStringTag      },
        { Tag::kArtist,               &MP4SinkBase::writeStringTag      },
        { Tag::kComment,              &MP4SinkBase::writeStringTag      },
        { Tag::kComposer,             &MP4SinkBase::writeStringTag      },
        { Tag::kCopyright,            &MP4SinkBase::writeStringTag      },
        { Tag::kDate,                 &MP4SinkBase::writeStringTag      },
        { Tag::kDescription,          &MP4SinkBase::writeStringTag      },
        { Tag::kGrouping,             &MP4SinkBase::writeStringTag      },
        { Tag::kLongDescription,      &MP4SinkBase::writeStringTag      },
        { Tag::kLyrics,               &MP4SinkBase::writeStringTag      },
        { Tag::kTitle,                &MP4SinkBase::writeStringTag      },
        { Tag::kTool,                 &MP4SinkBase::writeStringTag      },
        { Tag::kTrack,                &MP4SinkBase::writeTrackTag       },
        { Tag::kDisk,                 &MP4SinkBase::writeDiskTag        },
        { Tag::kGenre,                &MP4SinkBase::writeGenreTag       },
        { Tag::kGenreID3,             &MP4SinkBase::writeGenreTag       },
        { Tag::kCompilation,          &MP4SinkBase::writeInt8Tag        },
        { Tag::kTempo,                &MP4SinkBase::writeInt16Tag       },
        { Tag::kTvSeason,             &MP4SinkBase::writeInt32Tag       },
        { Tag::kTvEpisode,            &MP4SinkBase::writeInt32Tag       },
        { Tag::kPodcast,              &MP4SinkBase::writeInt8Tag        },
        { Tag::kHDVideo,              &MP4SinkBase::writeInt8Tag        },
        { Tag::kMediaType,            &MP4SinkBase::writeMediaTypeTag   },
        { Tag::kContentRating,        &MP4SinkBase::writeRatingTag      },
        { Tag::kGapless,              &MP4SinkBase::writeInt8Tag        },
        { Tag::kiTunesAccountType,    &MP4SinkBase::writeAccountTypeTag },
        { Tag::kiTunesCountry,        &MP4SinkBase::writeCountryCodeTag },
        { Tag::kcontentID,            &MP4SinkBase::writeInt32Tag       },
        { Tag::kartistID,             &MP4SinkBase::writeInt32Tag       },
        { Tag::kplaylistID,           &MP4SinkBase::writeInt64Tag       },
        { Tag::kgenreID,              &MP4SinkBase::writeInt32Tag       },
        { Tag::kcomposerID,           &MP4SinkBase::writeInt32Tag       },
        { 'apID',                     &MP4SinkBase::writeStringTag      },
        { 'catg',                     &MP4SinkBase::writeStringTag      },
        { 'keyw',                     &MP4SinkBase::writeStringTag      },
        { 'purd',                     &MP4SinkBase::writeStringTag      },
        { 'purl',                     &MP4SinkBase::writeStringTag      },
        { 'soaa',                     &MP4SinkBase::writeStringTag      },
        { 'soal',                     &MP4SinkBase::writeStringTag      },
        { 'soar',                     &MP4SinkBase::writeStringTag      },
        { 'soco',                     &MP4SinkBase::writeStringTag      },
        { 'sonm',                     &MP4SinkBase::writeStringTag      },
        { 'sosn',                     &MP4SinkBase::writeStringTag      },
        { 'tven',                     &MP4SinkBase::writeStringTag      },
        { 'tvnn',                     &MP4SinkBase::writeStringTag      },
        { 'tvsh',                     &MP4SinkBase::writeStringTag      },
        { 'xid ',                     &MP4SinkBase::writeStringTag      },
        { FOURCC('\xa9','e','n','c'), &MP4SinkBase::writeStringTag      },
        { 0,                          0                                 }
    };

    util::fourcc fourcc(fcc);
    for (handler_t *p = handlers; p->fcc; ++p) {
        if (fourcc == p->fcc) {
            (this->*p->mf)(fourcc.svalue, value);
            return;
        }
    }
}

void MP4SinkBase::writeLongTag(const std::string &key, const std::string &value)
{
    const uint8_t *v = reinterpret_cast<const uint8_t *>(value.c_str());
    auto type = mp4v2::impl::itmf::BT_UTF8;
    if (key == "Encoding Params")
        type = mp4v2::impl::itmf::BT_IMPLICIT;
    m_mp4file.SetMetadataFreeForm(key.c_str(), "com.apple.iTunes",
                                  v, value.size(), type);
}

void MP4SinkBase::writeTrackTag(const char *fcc, const std::string &value)
{
    int n, total = 0;
    if (std::sscanf(value.c_str(), "%d/%d", &n, &total) > 0)
        m_mp4file.SetMetadataTrack(n, total);
}
void MP4SinkBase::writeDiskTag(const char *fcc, const std::string &value)
{
    int n, total = 0;
    if (std::sscanf(value.c_str(), "%d/%d", &n, &total) > 0)
        m_mp4file.SetMetadataDisk(n, total);
}
void MP4SinkBase::writeGenreTag(const char *fcc, const std::string &value)
{
    char *endp;
    long n = std::strtol(value.c_str(), &endp, 10);
    if (endp != value.c_str() && *endp == 0)
        m_mp4file.SetMetadataGenre("gnre", n);
    else {
        n = static_cast<uint16_t>(enumGenreType.toType(value.c_str()));
        if (n != mp4v2::impl::itmf::GENRE_UNDEFINED)
            m_mp4file.SetMetadataGenre("gnre", n);
        else
            m_mp4file.SetMetadataString("\xa9""gen", value.c_str());
    }
}
void MP4SinkBase::writeMediaTypeTag(const char *fcc, const std::string &value)
{
    unsigned n;
    if (std::sscanf(value.c_str(), "%u", &n) != 1)
        n = static_cast<uint8_t>(enumStikType.toType(value.c_str()));
    m_mp4file.SetMetadataUint8(fcc, n);
}
void MP4SinkBase::writeRatingTag(const char *fcc, const std::string &value)
{
    unsigned n;
    if (std::sscanf(value.c_str(), "%u", &n) != 1)
        n = static_cast<uint8_t>(enumContentRating.toType(value.c_str()));
    m_mp4file.SetMetadataUint8(fcc, n);
}
void MP4SinkBase::writeAccountTypeTag(const char *fcc, const std::string &value)
{
    unsigned n;
    if (std::sscanf(value.c_str(), "%u", &n) != 1)
        n = static_cast<uint8_t>(enumAccountType.toType(value.c_str()));
    m_mp4file.SetMetadataUint8(fcc, n);
}
void MP4SinkBase::writeCountryCodeTag(const char *fcc, const std::string &value)
{
    unsigned n;
    if (std::sscanf(value.c_str(), "%u", &n) != 1)
        n = static_cast<uint32_t>(enumCountryCode.toType(value.c_str()));
    m_mp4file.SetMetadataUint32(fcc, n);
}
void MP4SinkBase::writeInt8Tag(const char *fcc, const std::string &value)
{
    int n;
    if (std::sscanf(value.c_str(), "%d", &n) == 1)
        m_mp4file.SetMetadataUint8(fcc, n);
}
void MP4SinkBase::writeInt16Tag(const char *fcc, const std::string &value)
{
    int n;
    if (std::sscanf(value.c_str(), "%d", &n) == 1)
        m_mp4file.SetMetadataUint16(fcc, n);
}
void MP4SinkBase::writeInt32Tag(const char *fcc, const std::string &value)
{
    int n;
    if (std::sscanf(value.c_str(), "%d", &n) == 1)
        m_mp4file.SetMetadataUint32(fcc, n);
}
void MP4SinkBase::writeInt64Tag(const char *fcc, const std::string &value)
{
    int64_t n;
    if (std::sscanf(value.c_str(), "%lld", &n) == 1)
        m_mp4file.SetMetadataUint64(fcc, n);
}
void MP4SinkBase::writeStringTag(const char *fcc, const std::string &value)
{
    std::string s = strutil::normalize_crlf(value.c_str(), "\r\n");
    m_mp4file.SetMetadataString(fcc, s.c_str());
}

MP4Sink::MP4Sink(const std::wstring &path,
                 const std::vector<uint8_t> &config,
                 bool temp)
        : MP4SinkBase(path, temp),
          m_gapless_mode(MODE_ITUNSMPB)
{
    std::memset(&m_priming_info, 0, sizeof m_priming_info);
    try {
        AudioStreamBasicDescription asbd;
        std::vector<uint32_t> channels;
        cautil::parseASC(config, &asbd, &channels);
        unsigned rate = asbd.mSampleRate;
        if (asbd.mFormatID == 'aach')
            rate /= 2;
        m_mp4file.SetTimeScale(rate);
        m_track_id = m_mp4file.AddAudioTrack(rate, 1024,
                                             MP4_MPEG4_AUDIO_TYPE);
        /*
         * According to ISO 14496-12 8.16.3, 
         * ChannelCount of AusioSampleEntry is either 1 or 2.
         */
        m_mp4file.SetIntegerProperty(
                "moov.trak.mdia.minf.stbl.stsd.mp4a.channels",
                asbd.mChannelsPerFrame);
        /* Looks like iTunes sets upsampled scale here */
        if (asbd.mFormatID == 'aach') { 
            uint64_t scale = static_cast<uint64_t>(rate) << 17;
            m_mp4file.SetIntegerProperty(
                "moov.trak.mdia.minf.stbl.stsd.mp4a.timeScale",
                scale);
        }
        m_mp4file.SetTrackESConfiguration(m_track_id, &config[0],
                                          config.size());
    } catch (mp4v2::impl::Exception *e) {
        handle_mp4error(e);
    }
}

void MP4Sink::writeTags()
{
    MP4TrackId tid = m_mp4file.FindTrackId(0);
    MP4SampleId nframes = m_mp4file.GetTrackNumberOfSamples(tid);
    if (nframes) {
        uint64_t duration = m_mp4file.GetTrack(tid)->GetDuration();

        if (m_gapless_mode & MODE_ITUNSMPB) {
            std::string value =
                strutil::format(iTunSMPB_template,
                m_edit_start,
                uint32_t(duration - m_edit_start - m_edit_duration),
                uint32_t(m_edit_duration >> 32),
                uint32_t(m_edit_duration & 0xffffffff));
            m_tags["iTunSMPB"] = value;
        }
        if (m_gapless_mode & MODE_EDTS) {
            MP4EditId eid = m_mp4file.AddTrackEdit(tid);
            m_mp4file.SetTrackEditMediaStart(tid, eid, m_edit_start);
            m_mp4file.SetTrackEditDuration(tid, eid, m_edit_duration);
            m_mp4file.CreateAudioSampleGroupDescription(tid, nframes);
        }
    }
    MP4SinkBase::writeTags();
}

ALACSink::ALACSink(const std::wstring &path,
        const std::vector<uint8_t> &magicCookie, bool temp)
        : MP4SinkBase(path, temp)
{
    try {
        std::vector<uint8_t> alac, chan;
        parseMagicCookieALAC(magicCookie, &alac, &chan);
        if (alac.size() != 24)
            throw std::runtime_error("Invalid ALACSpecificConfig!");
        if (chan.size() && chan.size() != 12)
            throw std::runtime_error("Invalid ALACChannelLayout!");

        m_track_id = m_mp4file.AddAlacAudioTrack(&alac[0],
                                                 chan.size() ? &chan[0] : 0);
    } catch (mp4v2::impl::Exception *e) {
        handle_mp4error(e);
    }
}

ADTSSink::ADTSSink(const std::wstring &path,
                   const std::vector<uint8_t> &cookie,
                   bool append)
    : m_fp(win32::fopen(path, append ? L"ab" : L"wb"))
{
    init(cookie);
}

ADTSSink::ADTSSink(const std::shared_ptr<FILE> &fp,
                   const std::vector<uint8_t> &cookie)
    : m_fp(fp)
{
    init(cookie);
}

void ADTSSink::writeSamples(const void *data, size_t length, size_t nsamples)
{
    BitStream bs;
    bs.put(0xfff, 12); // syncword
    bs.put(0, 1);  // ID(MPEG identifier). 0 for MPEG4, 1 for MPEG2
    bs.put(0, 2);  // layer. always 0
    bs.put(1, 1);  // protection absent. 1 means no CRC information
    bs.put(1, 2);  // profile, (MPEG-4 object type) - 1. 1 for AAC LC
    bs.put(m_sample_rate_index, 4); // sampling rate index
    bs.put(0, 1); // private bit
    bs.put(m_channel_config, 3); // channel configuration
    bs.put(0, 4); /*
                   * originaly/copy: 1
                   * home: 1
                   * copyright_identification_bit: 1
                   * copyright_identification_start: 1
                   */
    bs.put(length + m_pce_data.size() + 7, 13); // frame_length
    bs.put(0x7ff, 11); // adts_buffer_fullness, 0x7ff for VBR
    bs.put(0, 2); // number_of_raw_data_blocks_in_frame
    bs.byteAlign();

    write(bs.data(), 7);
    if (m_pce_data.size())
        write(&m_pce_data[0], m_pce_data.size());
    write(data, length);
}

void ADTSSink::init(const std::vector<uint8_t> &config)
{
    m_seekable = win32::is_seekable(fileno(m_fp.get()));
    unsigned rate;
    size_t off = parseDecSpecificConfig(config, &m_sample_rate_index, &rate,
                                        &m_channel_config);

    /* keep program config element stored in GASpecificConfig */
    if (m_channel_config == 0 && config.size() * 8 > off) {
        BitStream ibs(config.data(), config.size());
        ibs.advance(off + 3);
        BitStream obs;
        obs.put(5, 3); /* ID_PCE */
        obs.copy(ibs, 4+2+4); /* element_instance_tag, object_type, sf_index */

        /* number of channels */
        unsigned nfront, nside, nback, nlfe, ndata, ncc;
        nfront = obs.copy(ibs, 4);
        nside  = obs.copy(ibs, 4);
        nback  = obs.copy(ibs, 4);
        nlfe   = obs.copy(ibs, 2);
        ndata  = obs.copy(ibs, 3);
        ncc    = obs.copy(ibs, 4);

        if (obs.copy(ibs, 1)) obs.copy(ibs, 4); /* mono_mixdown */
        if (obs.copy(ibs, 1)) obs.copy(ibs, 4); /* stereo_mixdown */
        if (obs.copy(ibs, 1)) obs.copy(ibs, 2+1); /* matrix_mixdown */

        /* channel data */
        for (int i = 0; i < nfront + nside + nback; ++i)
            obs.copy(ibs, 1+4);
        for (int i = 0; i < nlfe + ndata; ++i)
            obs.copy(ibs, 4);
        for (int i = 0; i < ncc; ++i)
            obs.copy(ibs, 1+4);

        obs.byteAlign();
        obs.put(0, 8); /* comment_field_bytes */

        std::copy(obs.data(), obs.data() + obs.position() / 8,
                  std::back_inserter(m_pce_data));
    }
}
