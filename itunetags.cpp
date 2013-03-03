#include <algorithm>
#include <riff/aiff/aifffile.h>
#include <ape/apefile.h>
#include <ape/apetag.h>
#include <mpeg/mpegfile.h>
#include <mpeg/id3v1/id3v1genres.h>
#include <mpeg/id3v2/id3v2framefactory.h>
#include <mpeg/id3v2/frames/textidentificationframe.h>
#include "itunetags.h"
#ifdef _WIN32
#include "win32util.h"
#endif
#include "strutil.h"
#include "mp4v2wrapper.h"
#include "wicimage.h"
#include "cuesheet.h"
#include "src/itmf/type.h"
#include "taglibhelper.h"

using mp4v2::impl::MP4File;
using mp4v2::impl::MP4Atom;
using mp4v2::impl::Exception;
using mp4v2::impl::itmf::enumGenreType;
using mp4v2::impl::itmf::enumStikType;
using mp4v2::impl::itmf::enumAccountType;
using mp4v2::impl::itmf::enumCountryCode;
using mp4v2::impl::itmf::enumContentRating;

class ShortTagWriter {
    MP4FileX &m_file;
public:
    ShortTagWriter(MP4FileX &f) : m_file(f) {}
    void operator()(const std::pair<uint32_t, std::wstring> &kv)
    {
        writeTag(kv.first, kv.second);
    }
    void writeTag(uint32_t key, const std::wstring &value)
    {
        std::string sv = strutil::w2us(value);
        util::fourcc fcc(key);

        struct handler_t {
            uint32_t fcc;
            void (ShortTagWriter::*mf)(const char *, const char *);
        } handlers[] = {
            { Tag::kAlbum,                &ShortTagWriter::setString      },
            { Tag::kAlbumArtist,          &ShortTagWriter::setString      },
            { Tag::kArtist,               &ShortTagWriter::setString      },
            { Tag::kComment,              &ShortTagWriter::setString      },
            { Tag::kComposer,             &ShortTagWriter::setString      },
            { Tag::kCopyright,            &ShortTagWriter::setString      },
            { Tag::kDate,                 &ShortTagWriter::setString      },
            { Tag::kDescription,          &ShortTagWriter::setString      },
            { Tag::kGrouping,             &ShortTagWriter::setString      },
            { Tag::kLongDescription,      &ShortTagWriter::setString      },
            { Tag::kLyrics,               &ShortTagWriter::setString      },
            { Tag::kSubTitle,             &ShortTagWriter::setString      },
            { Tag::kTitle,                &ShortTagWriter::setString      },
            { Tag::kTool,                 &ShortTagWriter::setString      },
            { Tag::kTrack,                &ShortTagWriter::setTrack       },
            { Tag::kDisk,                 &ShortTagWriter::setDisk        },
            { Tag::kGenre,                &ShortTagWriter::setGenre       },
            { Tag::kGenreID3,             &ShortTagWriter::setGenre       },
            { Tag::kCompilation,          &ShortTagWriter::setInt8        },
            { Tag::kTempo,                &ShortTagWriter::setInt16       },
            { Tag::kArtwork,              &ShortTagWriter::ignore         },
            { Tag::kTvSeason,             &ShortTagWriter::setInt32       },
            { Tag::kTvEpisode,            &ShortTagWriter::setInt32       },
            { Tag::kPodcast,              &ShortTagWriter::setInt8        },
            { Tag::kHDVideo,              &ShortTagWriter::setInt8        },
            { Tag::kMediaType,            &ShortTagWriter::setMediaType   },
            { Tag::kContentRating,        &ShortTagWriter::setRating      },
            { Tag::kGapless,              &ShortTagWriter::setInt8        },
            { Tag::kiTunesAccountType,    &ShortTagWriter::setAccountType },
            { Tag::kiTunesCountry,        &ShortTagWriter::setCountryCode },
            { Tag::kcontentID,            &ShortTagWriter::setInt32       },
            { Tag::kartistID,             &ShortTagWriter::setInt32       },
            { Tag::kplaylistID,           &ShortTagWriter::setInt64       },
            { Tag::kgenreID,              &ShortTagWriter::setInt32       },
            { Tag::kcomposerID,           &ShortTagWriter::setInt32       },
            { 'apID',                     &ShortTagWriter::setString      },
            { 'catg',                     &ShortTagWriter::setString      },
            { 'keyw',                     &ShortTagWriter::setString      },
            { 'purd',                     &ShortTagWriter::setString      },
            { 'purl',                     &ShortTagWriter::setString      },
            { 'soaa',                     &ShortTagWriter::setString      },
            { 'soal',                     &ShortTagWriter::setString      },
            { 'soar',                     &ShortTagWriter::setString      },
            { 'soco',                     &ShortTagWriter::setString      },
            { 'sonm',                     &ShortTagWriter::setString      },
            { 'sosn',                     &ShortTagWriter::setString      },
            { 'tven',                     &ShortTagWriter::setString      },
            { 'tvnn',                     &ShortTagWriter::setString      },
            { 'tvsh',                     &ShortTagWriter::setString      },
            { 'xid ',                     &ShortTagWriter::setString      },
            { FOURCC('\xa9','e','n','c'), &ShortTagWriter::setString      },
            { 0,                          0                               }
        };
        for (handler_t *p = handlers; p->fcc; ++p) {
            if (fcc == p->fcc) {
                (this->*p->mf)(fcc.svalue, sv.c_str());
                return;
            }
        }
    }
private:
    void setTrack(const char *fcc, const char *value)
    {
        int n, total = 0;
        if (std::sscanf(value, "%d/%d", &n, &total) > 0)
            m_file.SetMetadataTrack(n, total);
    }
    void setDisk(const char *fcc, const char *value)
    {
        int n, total = 0;
        if (std::sscanf(value, "%d/%d", &n, &total) > 0)
            m_file.SetMetadataDisk(n, total);
    }
    void setGenre(const char *fcc, const char *value)
    {
        char *endp;
        long n = std::strtol(value, &endp, 10);
        if (endp != value && *endp == 0)
            m_file.SetMetadataGenre("gnre", n);
        else {
            n = static_cast<uint16_t>(enumGenreType.toType(value));
            if (n != mp4v2::impl::itmf::GENRE_UNDEFINED)
                m_file.SetMetadataGenre("gnre", n);
            else
                m_file.SetMetadataString("\xa9""gen", value);
        }
    }
    void setMediaType(const char *fcc, const char *value)
    {
        unsigned n;
        if (std::scanf("%u", &n) != 1)
            n = static_cast<uint8_t>(enumStikType.toType(value));
        m_file.SetMetadataUint8(fcc, n);
    }
    void setRating(const char *fcc, const char *value)
    {
        unsigned n;
        if (std::scanf("%u", &n) != 1)
            n = static_cast<uint8_t>(enumContentRating.toType(value));
        m_file.SetMetadataUint8(fcc, n);
    }
    void setAccountType(const char *fcc, const char *value)
    {
        unsigned n;
        if (std::scanf("%u", &n) != 1)
            n = static_cast<uint8_t>(enumAccountType.toType(value));
        m_file.SetMetadataUint8(fcc, n);
    }
    void setCountryCode(const char *fcc, const char *value)
    {
        unsigned n;
        if (std::scanf("%u", &n) != 1)
            n = static_cast<uint8_t>(enumCountryCode.toType(value));
        m_file.SetMetadataUint32(fcc, n);
    }
    void setInt8(const char *fcc, const char *value)
    {
        int n;
        if (std::sscanf(value, "%d", &n) == 1)
            m_file.SetMetadataUint8(fcc, n);
    }
    void setInt16(const char *fcc, const char *value)
    {
        int n;
        if (std::sscanf(value, "%d", &n) == 1)
            m_file.SetMetadataUint16(fcc, n);
    }
    void setInt32(const char *fcc, const char *value)
    {
        int n;
        if (std::sscanf(value, "%d", &n) == 1)
            m_file.SetMetadataUint32(fcc, n);
    }
    void setInt64(const char *fcc, const char *value)
    {
        int64_t n;
        if (std::sscanf(value, "%lld", &n) == 1)
            m_file.SetMetadataUint64(fcc, n);
    }
    void setString(const char *fcc, const char *value)
    {
        std::string s = strutil::normalize_crlf(value, "\r\n");
        m_file.SetMetadataString(fcc, s.c_str());
    }
    void ignore(const char *fcc, const char *value)
    {
    }
};

class LongTagWriter {
    MP4FileX &m_file;
public:
    LongTagWriter(MP4FileX &f): m_file(f) {}
    void operator()(const std::pair<std::string, std::wstring> &kv)
    {
        writeTag(kv.first, kv.second);
    }
    void writeTag(const std::string &key, const std::wstring &value)
    {
        std::string sv = strutil::w2us(value);
        try {
            m_file.SetMetadataFreeForm(key.c_str(),
                "com.apple.iTunes",
                reinterpret_cast<const uint8_t*>(sv.c_str()),
                sv.size());
        } catch (Exception *e) {
            handle_mp4error(e);
        }
    }
};

void TagEditor::save(MP4FileX &file)
{
    try {
        if ((m_encoder_delay || m_padding) && m_nsamples) {
            if (m_gapless_mode & MODE_ITUNSMPB) {
                std::wstring value = strutil::format(iTunSMPB_template,
                                            m_encoder_delay,
                                            m_padding,
                                            uint32_t(m_nsamples >> 32),
                                            uint32_t(m_nsamples & 0xffffffff));
                m_long_tags["iTunSMPB"] = value;
            }
            if (m_gapless_mode & MODE_EDTS) {
                MP4TrackId tid = file.FindTrackId(0);
                MP4EditId eid = file.AddTrackEdit(tid);
                file.SetTrackEditMediaStart(tid, eid, m_encoder_delay);
                file.SetTrackEditDuration(tid, eid, m_nsamples);
                MP4SampleId count = file.GetTrackNumberOfSamples(tid);
                file.CreateAudioSampleGroupDescription(tid, count);
            }
        }
        if (m_chapters.size()) {
            uint64_t timeScale = file.GetIntegerProperty("moov.mvhd.timeScale");
            MP4TrackId track = file.AddChapterTextTrack(1);
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
            double off = static_cast<double>(m_encoder_delay) / timeScale;
            std::vector<chapters::entry_t>::const_iterator chap;
            for (chap = m_chapters.begin(); chap != m_chapters.end(); ++chap) {
                std::string name = strutil::w2us(chap->first);
                const char *namep = name.c_str();
                file.AddChapter(track, chap->second * timeScale + 0.5, namep);
                int64_t stamp = off * 10000000.0 + 0.5;
                file.AddNeroChapter(stamp, namep);
                off += chap->second;
            }
        }
        std::for_each(m_tags.begin(), m_tags.end(), ShortTagWriter(file));
        std::for_each(m_long_tags.begin(), m_long_tags.end(),
                      LongTagWriter(file));
    } catch (Exception *e) {
        handle_mp4error(e);
    }
}

void TagEditor::saveArtworks(MP4FileX &file)
{
#ifdef _WIN32
    try {
        for (size_t i = 0; i < m_artworks.size(); ++i) {
            uint64_t size;
            char *data =
                win32::load_with_mmap(m_artworks[i].c_str(), &size);
            std::shared_ptr<char> dataPtr(data, UnmapViewOfFile);
            mp4v2::impl::itmf::BasicType tc =
                mp4v2::impl::itmf::computeBasicType(data, size);
            if (tc == mp4v2::impl::itmf::BT_IMPLICIT)
                throw std::runtime_error("Unknown artwork image type");
            std::vector<char> vec;
            if (m_artwork_size) {
                bool res = WICConvertArtwork(data, size, m_artwork_size, &vec);
                if (res) {
                    data = &vec[0];
                    size = vec.size();
                }
            }
            file.SetMetadataArtwork("covr", data, size);
        }
    } catch (Exception *e) {
        handle_mp4error(e);
    }
#endif
}

namespace ID3 {
    const char *known_keys[][2] = {
        { "TALB", "album"               },
        { "TBPM", "bpm"                 },
        { "TCMP", "itunescompilation"   },
        { "TCOM", "composer"            },
        { "TCON", "genre"               },
        { "TCOP", "copyright"           },
        { "TDRC", "date"                },
        { "TIT1", "content group"       },
        { "TIT2", "title"               },
        { "TIT3", "subtitle"            },
        { "TPE1", "artist"              },
        { "TPE2", "album artist"        },
        { "TPOS", "discnumber"          },
        { "TRCK", "tracknumber"         },
        { "TSO2", "albumartistsort"     },
        { "TSOA", "albumsort"           },
        { "TSOC", "composersort"        },
        { "TSOP", "artistsort"          },
        { "TSOT", "titlesort"           },
    };
    void fetchID3v2Tags(TagLib::ID3v2::Tag *tag,
                        std::map<uint32_t, std::wstring> *result)
    {
        using TagLib::ID3v2::Frame;
        using TagLib::ID3v2::FrameList;
        using TagLib::ID3v2::TextIdentificationFrame;

        typedef const char *entry_t[2];
        struct comparator {
            static int call(const void *k, const void *v)
            {
                const char *key = static_cast<const char *>(k);
                const entry_t *ent = static_cast<const entry_t *>(v);
                return std::strcmp(key, (*ent)[0]);
            }
        };

        std::map<std::string, std::string> tags;

        const FrameList &frameList = tag->frameList();
        FrameList::ConstIterator it;
        for (it = frameList.begin(); it != frameList.end(); ++it) {
            Frame *frame = *it;
            TagLib::ByteVector vID = frame->frameID();
            std::string sID(vID.data(), vID.data() + vID.size());

            if (sID == "TXXX") {
                TextIdentificationFrame *txframe
                    = dynamic_cast<TextIdentificationFrame*>(frame);
                TagLib::StringList fields = txframe->fieldList();
                std::wstring k = fields.begin()->toWString();
                std::wstring v = (++fields.begin())->toWString();
                tags[strutil::w2us(k)] = strutil::w2us(v);
            } else {
                entry_t *entry = static_cast<entry_t*>(
                    std::bsearch(sID.c_str(), known_keys,
                                 util::sizeof_array(known_keys),
                                 sizeof(known_keys[0]), comparator::call));
                if (entry) {
                    std::wstring value = frame->toString().toWString();
                    tags[(*entry)[1]] = strutil::w2us(value);
                }
            }
        }
        Vorbis::ConvertToItunesTags(tags, result);
    }
    void fetchAiffID3Tags(int fd, std::map<uint32_t, std::wstring> *result)
    {
        util::FilePositionSaver _(fd);
        lseek(fd, 0, SEEK_SET);
        TagLibX::FDIOStreamReader stream(fd);
        TagLib::RIFF::AIFF::File file(&stream, false);
        TagLib::ID3v2::Tag *tag = file.tag();
        fetchID3v2Tags(tag, result);
    }
    void fetchMPEGID3Tags(int fd, std::map<uint32_t, std::wstring> *result)
    {
        util::FilePositionSaver _(fd);
        lseek(fd, 0, SEEK_SET);
        TagLibX::FDIOStreamReader stream(fd);
        TagLib::MPEG::File file(&stream,
                                TagLib::ID3v2::FrameFactory::instance(),
                                false);
        TagLib::ID3v2::Tag *tag = file.ID3v2Tag();
        fetchID3v2Tags(tag, result);
    }
}

namespace Vorbis {
    const Tag::NameIDMap tagNameMap[] = {
        { "album",                      Tag::kAlbum             },
        { "album artist",               Tag::kAlbumArtist       },
        { "albumartist",                Tag::kAlbumArtist       },
        { "albumartistsort",            'soaa'                  },
        { "albumartistsortorder",       'soaa'                  },
        { "albumsort",                  'soal'                  },
        { "albumsortorder",             'soal'                  },
        { "artist",                     Tag::kArtist            },
        { "artistsort",                 'soar'                  },
        { "artistsortorder",            'soar'                  },
        { "band",                       Tag::kAlbumArtist       },
        { "bpm",                        Tag::kTempo             },
        { "comment",                    Tag::kComment           },
        { "compilation",                Tag::kCompilation       },
        { "composer",                   Tag::kComposer          },
        { "composersort",               'soco'                  },
        { "composersortorder",          'soco'                  },
        { "content group",              Tag::kGrouping          },
        { "contentgroup",               Tag::kGrouping          },
        { "copyright",                  Tag::kCopyright         },
        { "date",                       Tag::kDate              },
        { "disc",                       Tag::kDisk              },
        { "disc number",                Tag::kDisk              },
        { "discnumber",                 Tag::kDisk              },
        { "genre",                      Tag::kGenre             },
        { "grouping",                   Tag::kGrouping          },
        { "itunescompilation",          Tag::kCompilation       },
        { "lyrics",                     Tag::kLyrics            },
        { "subtitle",                   Tag::kSubTitle          },
        { "title",                      Tag::kTitle             },
        { "titlesort",                  'sonm'                  },
        { "titlesortorder",             'sonm'                  },
        { "track",                      Tag::kTrack             },
        { "track number",               Tag::kTrack             },
        { "tracknumber",                Tag::kTrack             },
        { "unsynced lyrics",            Tag::kLyrics            },
        { "year",                       Tag::kDate              },
    };
    uint32_t GetIDFromTagName(const char *name)
    {
        struct comparator {
            static int call(const void *k, const void *v)
            {
                const char *key = static_cast<const char *>(k);
                const Tag::NameIDMap *ent
                    = static_cast<const Tag::NameIDMap *>(v);
                return std::strcmp(key, ent->name);
            }
        };
        const Tag::NameIDMap *entry =
            static_cast<const Tag::NameIDMap *>(
                std::bsearch(name, tagNameMap, util::sizeof_array(tagNameMap),
                             sizeof(tagNameMap[0]), comparator::call));
        return entry ? entry->id : 0;
    }

    void processTotal(const std::map<std::string, std::string> &vc,
                      std::map<uint32_t, std::wstring> &itags)
    {
        std::map<std::string, std::string>::const_iterator vit;
        std::map<uint32_t, std::wstring>::iterator iit;

#define MERGE_TAG(x, y) \
        do { x->second = strutil::format(L"%d/%d", \
                                         _wtoi(x->second.c_str()), \
                                         atoi(y->second.c_str())); \
        } while (0)

        if ((iit = itags.find(Tag::kTrack)) != itags.end()) {
            if ((vit = vc.find("totaltracks")) != vc.end())
                MERGE_TAG(iit, vit);
            else if ((vit = vc.find("tracktotal")) != vc.end())
                MERGE_TAG(iit, vit);
        }
        if ((iit = itags.find(Tag::kDisk)) != itags.end()) {
            if ((vit = vc.find("totaldiscs")) != vc.end())
                MERGE_TAG(iit, vit);
            else if ((vit = vc.find("disctotal")) != vc.end())
                MERGE_TAG(iit, vit);
        }
#undef MERGE_TAG
    }
    void ConvertToItunesTags(const std::map<std::string, std::string> &vc,
                             std::map<uint32_t, std::wstring> *itags)
    {
        std::map<std::string, std::string>::const_iterator it;
        std::map<std::string, std::string> lcvc;
        std::map<uint32_t, std::wstring> result;
        uint32_t id;
        for (it = vc.begin(); it != vc.end(); ++it)
            lcvc[strutil::slower(it->first)] = it->second;

        for (it = lcvc.begin(); it != lcvc.end(); ++it) {
            if ((id = GetIDFromTagName(it->first.c_str())) > 0)
                result[id] = strutil::us2w(it->second);
        }
        processTotal(lcvc, result);
        itags->swap(result);
    }
}

namespace mp4a
{
    std::wstring parseValue(uint32_t fcc, const MP4ItmfData &data)
    {
        uint8_t *value = data.value;

        if (fcc == Tag::kGenreID3) {
            unsigned v = (value[0] << 8) | value[1];
            return strutil::format(L"%u", v);
        } else if (fcc == Tag::kDisk || fcc == Tag::kTrack) {
            unsigned index = (value[2] << 8) | value[3];
            unsigned total = (value[4] << 8) | value[5];
            return strutil::format(L"%u/%u", index, total);
        } else if (data.typeCode == MP4_ITMF_BT_INTEGER) {
            if (data.valueSize == 8) {
                uint32_t high, low;
                high = (value[0]<<24)|(value[1]<<16)|(value[2]<<8)|value[3];
                low  = (value[4]<<24)|(value[5]<<16)|(value[6]<<8)|value[7];
                uint64_t value = (static_cast<uint64_t>(high) << 32) | low;
                return strutil::format(L"%lld", value);
            }
            int v;
            if (data.valueSize == 1)
                v = value[0];
            else if (data.valueSize == 2)
                v = (value[0] << 8) | value[1];
            else if (data.valueSize == 4)
                v = (value[0]<<24)|(value[1]<<16)|(value[2]<<8)|value[3];
            else
                return L"";
            return strutil::format(L"%d", v);
        } else if (data.typeCode == MP4_ITMF_BT_UTF8) {
            char *vp = reinterpret_cast<char*>(value);
            std::string s(vp, vp + data.valueSize);
            return strutil::us2w(s);
        }
        return L"";
    }

    void fetchTags(MP4FileX &file, std::map<uint32_t, std::wstring> *shortTags,
                   std::map<std::string, std::wstring> *longTags)
    {
        std::map<uint32_t, std::wstring> stags;
        std::map<std::string, std::wstring> ltags;
        try {
            MP4ItmfItemList *itemlist =
                mp4v2::impl::itmf::genericGetItems(file);
            if (!itemlist)
                return;
            std::shared_ptr<MP4ItmfItemList> __delete_later__(
                    itemlist, mp4v2::impl::itmf::genericItemListFree);
            for (size_t i = 0; i < itemlist->size; ++i) {
                MP4ItmfItem &item = itemlist->elements[i];
                uint32_t fcc = util::fourcc(item.code);
                MP4ItmfData &data = item.dataList.elements[0];
                if (!data.value)
                    continue;
                std::wstring v = parseValue(fcc, data);
                if (!v.empty()) {
                    if (fcc == '----')
                        ltags[item.name] = v;
                    else
                        stags[fcc] = v;
                }
            }
            if (shortTags) shortTags->swap(stags);
            if (longTags) longTags->swap(ltags);
        } catch (mp4v2::impl::Exception *e) {
            handle_mp4error(e);
        }
    }
}
