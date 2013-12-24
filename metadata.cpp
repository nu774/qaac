#include <algorithm>
#include <riff/aiff/aifffile.h>
#include <ape/apefile.h>
#include <ape/apetag.h>
#include <mpeg/mpegfile.h>
#include <mpeg/id3v1/id3v1genres.h>
#include <mpeg/id3v2/id3v2framefactory.h>
#include <mpeg/id3v2/frames/textidentificationframe.h>
#include "metadata.h"
#ifdef _WIN32
#include "win32util.h"
#endif
#include "strutil.h"
#include "mp4v2wrapper.h"
#include "cuesheet.h"
#include "taglibhelper.h"

namespace TextBasedTag {
    const char *known_keys[][2] = {
        { "album",                      "album"                    },
        { "albumartist",                "ALBUM ARTIST"             },
        { "albumartistsort",            "ALBUMARTISTSORT"          },
        { "albumartistsortorder",       "ALBUMARTISTSORT"          },
        { "albumsort",                  "ALBUMSORT"                },
        { "albumsortorder",             "ALBUMSORT"                },
        { "artist",                     "artist"                   },
        { "artistsort",                 "ARTISTSORT"               },
        { "artistsortorder",            "ARTISTSORT"               },
        { "band",                       "BAND"                     },
        { "bpm",                        "tempo"                    },
        { "comment",                    "comments"                 },
        { "comments",                   "comments"                 },
        { "compilation",                "iTunes:cpil"              },
        { "composer",                   "composer"                 },
        { "composersort",               "COMPOSERSORT"             },
        { "composersortorder",          "COMPOSERSORT"             },
        { "contentgroup",               "GROUPING"                 },
        { "copyright",                  "copyright"                },
        { "date",                       "year"                     },
        { "description",                "DESCRIPTION"              },
        { "disc",                       "DISC NUMBER"              },
        { "discnumber",                 "DISC NUMBER"              },
        { "disctotal",                  "TOTAL DISCS"              },
        { "encodedby",                  "encoding application"     },
        { "encoder",                    "encoding application"     },
        { "encodingapplication",        "encoding application"     },
        { "genre",                      "genre"                    },
        { "grouping",                   "GROUPING"                 },
        { "itunes:apid"                 "iTunes:apid"              },
        { "itunes:akid"                 "iTunes:akid"              },
        { "itunes:atid"                 "iTunes:atid"              },
        { "itunes:catg"                 "iTunes:catg"              },
        { "itunes:cmid"                 "iTunes:cmid"              },
        { "itunes:cnid"                 "iTunes:cnid"              },
        { "itunes:cpil"                 "iTunes:cpil"              },
        { "itunes:geid"                 "iTunes:geid"              },
        { "itunes:hdvd"                 "iTunes:hdvd"              },
        { "itunes:keyw"                 "iTunes:keyw"              },
        { "itunes:pcst"                 "iTunes:pcst"              },
        { "itunes:pgap"                 "iTunes:pgap"              },
        { "itunes:plid"                 "iTunes:plid"              },
        { "itunes:purl"                 "iTunes:purl"              },
        { "itunes:rtng"                 "iTunes:rtng"              },
        { "itunes:sfid"                 "iTunes:sfid"              },
        { "itunes:sosn"                 "iTunes:sosn"              },
        { "itunes:stik"                 "iTunes:stik"              },
        { "itunes:tvep"                 "iTunes:tvep"              },
        { "itunes:tvnn"                 "iTunes:tvnn"              },
        { "itunes:tvsh"                 "iTunes:tvsh"              },
        { "itunes:tvsn"                 "iTunes:tvsn"              },
        { "itunes:xid"                  "iTunes:xid"               },
        { "itunescompilation",          "iTunes:cpil"              },
        { "longdescription",            "LONG DESCRIPTION"         },
        { "lyrics",                     "LYRICS"                   },
        { "performer",                  "PERFORMER"                },
        { "recorddate",                 "recorded date"            },
        { "recordeddate",               "recorded date"            },
        { "songwriter",                 "composer"                 },
        { "synopsis",                   "LONG DESCRIPTION"         },
        { "tempo",                      "tempo"                    },
        { "timesignature",              "time signature"           },
        { "title",                      "title"                    },
        { "titlesort",                  "TITLESORT"                },
        { "titlesortorder",             "TITLESORT"                },
        { "totaldiscs",                 "TOTAL DISCS"              },
        { "totaltracks",                "TOTAL TRACKS"             },
        { "track",                      "track number"             },
        { "tracknumber",                "track number"             },
        { "tracktotal",                 "TOTAL TRACKS"             },
        { "tvepisodeid",                "iTunes:tven"              },
        { "tvnetwork",                  "iTunes:tvnn"              },
        { "tvshow",                     "iTunes:tvsh"              },
        { "tvshowsort",                 "iTunes:sosn"              },
        { "unsyncedlyrics",             "LYRICS"                   },
        { "year",                       "year"                     },
        { "yearrel",                    "year"                     },
    };
    std::string normalizeTagName(const char *name)
    {
        typedef const char *entry_t[2];
        std::string sname;
        bool lower = true;
        for (const char *s = name; *s; ++s) {
            unsigned char c = *s;
            if (c != ' ' && c != '-' && c != '_')
                sname.push_back(tolower(c));
            if (isalpha(c) && !islower(c))
                lower = false;
        }
        entry_t search = { sname.c_str(), 0 };
        entry_t *end = known_keys + util::sizeof_array(known_keys);
        auto entry =
            static_cast<entry_t *>(
                std::lower_bound(known_keys, end, search,
                                 [](const entry_t &a, const entry_t &b) ->int {
                                    return std::strcmp(a[0], b[0]) < 0;
                                 }));
        return entry < end && !std::strcmp((*entry)[0], search[0])
                   ? (*entry)[1] : lower ? strutil::supper(name) : name;
    }
    void normalizeTags(const std::map<std::string, std::string> &src,
                       std::map<std::string, std::string> *dst)
    {
        std::map<std::string, std::string> result;
        unsigned track = 0, track_total = 0;
        unsigned disc = 0, disc_total = 0;

        std::for_each(src.begin(), src.end(), [&](decltype(*src.begin()) tag) {
            std::string key = normalizeTagName(tag.first.c_str());
            const char *sv = tag.second.c_str();
            if (key == "track number")
                sscanf(sv, "%u/%u", &track, &track_total);
            else if (key == "TOTAL TRACKS")
                sscanf(sv, "%u", &track_total);
            else if (key == "DISC NUMBER")
                sscanf(sv, "%u/%u", &disc, &disc_total);
            else if (key == "TOTAL DISCS")
                sscanf(sv, "%u", &disc_total);
            else
                result[key] = tag.second;
        });
        if (track) {
            if (track_total)
                result["track number"] =
                    strutil::format("%u/%u", track, track_total);
            else
                result["track number"] = strutil::format("%u", track);
        }
        if (disc) {
            if (disc_total)
                result["DISC NUMBER"] =
                    strutil::format("%u/%u", disc, disc_total);
            else
                result["DISC NUMBER"] = strutil::format("%u", disc);
        }
        dst->swap(result);
    }
}

namespace ID3 {
    const char *known_keys[][2] = {
        { "TALB", "album"                       },
        { "TBPM", "tempo"                       },
        { "TCMP", "iTunes:cpil"                 },
        { "TCOM", "composer"                    },
        { "TCON", "genre"                       },
        { "TCOP", "copyright"                   },
        {" TDOR", "ORIGINAL RELEASE DATE"       },
        { "TDRC", "recorded date"               },
        { "TEXT", "lyricist"                    },
        { "TIT1", "GROUPING"                    },
        { "TIT2", "title"                       },
        { "TIT3", "SUBTITLE"                    },
        { "TKEY", "key signature"               },
        { "TLAN", "LANGUAGE"                    },
        { "TMED", "MEDIA TYPE"                  },
        { "TOAL", "ORIGINAL ALBUM"              },
        { "TOPE", "ORIGINAL ARTIST"             },
        { "TOWN", "OWNER"                       },
        { "TPE1", "artist"                      },
        { "TPE2", "ALBUM ARTIST"                },
        { "TPE3", "CONDUCTOR"                   },
        { "TPE4", "REMIXER"                     },
        { "TPOS", "DISC NUMBER"                 },
        { "TPUB", "PUBLISHER"                   },
        { "TRCK", "track number"                },
        { "TRSN", "RADIO STATION"               },
        { "TRSO", "RADIO STATION OWNER"         },
        { "TSO2", "ALBUMARTISTSORT"             },
        { "TSOA", "ALBUMSORT"                   },
        { "TSOC", "COMPOSERSORT"                },
        { "TSOP", "ARTISTSORT"                  },
        { "TSOT", "TITLESORT"                   },
        { "TSRC", "ISRC"                        },
        { "TSST", "SET SUBTITLE"                },
    };
    void fetchID3v2Tags(TagLib::ID3v2::Tag *tag,
                        std::map<std::string, std::string> *result)
    {
        typedef const char *entry_t[2];
        std::map<std::string, std::string> tags;

        auto frameList = tag->frameList();
        std::for_each(frameList.begin(), frameList.end(),
                      [&](TagLib::ID3v2::Frame *frame) {
            auto vID = frame->frameID();
            std::string sID(vID.data(), vID.data() + vID.size());

            if (sID == "TXXX") {
                auto txframe =
                  dynamic_cast<TagLib::ID3v2::TextIdentificationFrame*>(frame);
                auto fields = txframe->fieldList();
                auto k = fields.begin()->toWString();
                auto v = (++fields.begin())->toWString();
                tags[strutil::w2us(k)] = strutil::w2us(v);
            } else {
                entry_t search = { sID.c_str(), 0 };
                entry_t *end = known_keys + util::sizeof_array(known_keys);
                auto entry =
                    std::lower_bound(known_keys, end, search,
                                     [](const entry_t &a,
                                        const entry_t &b) -> int
                                     { return std::strcmp(a[0], b[0]) < 0; });
                if (entry < end && !std::strcmp((*entry)[0], search[0])) {
                    auto value = frame->toString().toWString();
                    tags[(*entry)[1]] = strutil::w2us(value);
                }
            }
        });
        TextBasedTag::normalizeTags(tags, result);
    }
    void fetchAiffID3Tags(int fd, std::map<std::string, std::string> *result)
    {
        util::FilePositionSaver _(fd);
        lseek(fd, 0, SEEK_SET);
        TagLibX::FDIOStreamReader stream(fd);
        TagLib::RIFF::AIFF::File file(&stream, false);
        auto tag = file.tag();
        fetchID3v2Tags(tag, result);
    }
    void fetchMPEGID3Tags(int fd, std::map<std::string, std::string> *result)
    {
        util::FilePositionSaver _(fd);
        lseek(fd, 0, SEEK_SET);
        TagLibX::FDIOStreamReader stream(fd);
        TagLib::MPEG::File file(&stream,
                                TagLib::ID3v2::FrameFactory::instance(),
                                false);
        auto tag = file.ID3v2Tag();
        fetchID3v2Tags(tag, result);
    }
}

namespace M4A {
    enum { TAG_TOTAL_DISCS = 1, TAG_TOTAL_TRACKS = 2 };

    typedef struct fcc2name_t {
        uint32_t fcc;
        const char *name;
    } fcc2name_t;

    typedef struct name2fcc_t {
        const char *name;
        uint32_t fcc;
    } name2fcc_t;

    const fcc2name_t iTunes_fcc2name_map[] = {
        { 'aART',                     "ALBUM ARTIST"          }, 
        { 'akID',                     "iTunes:akID"          },
        { 'apID',                     "iTunes:apID"          },
        { 'atID',                     "iTunes:atID"          },
        { 'catg',                     "iTunes:catg"          },
        { 'cmID',                     "iTunes:cmID"          },
        { 'cnID',                     "iTunes:cnID"          },
        { 'cpil',                     "iTunes:cpil"          }, 
        { 'cprt',                     "copyright"            }, 
        { 'desc',                     "DESCRIPTION"          }, 
        { 'disk',                     "DISC NUMBER"          }, 
        { 'geID',                     "iTunes:geID"          },
        { 'gnre',                     "genre"                },
        { 'hdvd',                     "iTunes:hdvd"          },
        { 'keyw',                     "iTunes:keyw"          },
        { 'ldes',                     "LONG DESCRIPTION"     },
        { 'pcst',                     "iTunes:pcst"          },
        { 'pgap',                     "iTunes:pgap"          },
        { 'plID',                     "iTunes:plID"          },
        { 'purd',                     "iTunes:purd"          },
        { 'purl',                     "iTunes:purl"          },
        { 'rtng',                     "iTunes:rtng"          },
        { 'sfID',                     "iTunes:sfID"          },
        { 'soaa',                     "ALBUMARTISTSORT"      },
        { 'soal',                     "ALBUMSORT"            },
        { 'soar',                     "ARTISTSORT"           },
        { 'soco',                     "COMPOSERSORT"         },
        { 'sonm',                     "TITLESORT"            },
        { 'sosn',                     "iTunes:sosn"          },
        { 'stik',                     "iTunes:stik"          },
        { 'tmpo',                     "tempo"                },
        { 'trkn',                     "track number"         },
        { 'tvep',                     "iTunes:tvep"          },
        { 'tvnn',                     "iTunes:tvnn"          },
        { 'tvsh',                     "iTunes:tvsh"          },
        { 'tvsn',                     "iTunes:tvsn"          },
        { 'xid ',                     "iTunes:xid"           },
        { FOURCC('\xa9','A','R','T'), "artist"               },
        { FOURCC('\xa9','a','l','b'), "album"                },
        { FOURCC('\xa9','c','m','t'), "comments"             },
        { FOURCC('\xa9','d','a','y'), "year"                 },
        { FOURCC('\xa9','e','n','c'), "encoded by"           },
        { FOURCC('\xa9','g','e','n'), "genre"                },
        { FOURCC('\xa9','g','r','p'), "GROUPING"             },
        { FOURCC('\xa9','l','y','r'), "LYRICS"               },
        { FOURCC('\xa9','n','a','m'), "title"                },
        { FOURCC('\xa9','t','o','o'), "encoding application" },
        { FOURCC('\xa9','w','r','t'), "composer"             },
    };

    const name2fcc_t iTunes_name2fcc_map[] = {
        { "album",                      Tag::kAlbum                },
        { "albumartist",                Tag::kAlbumArtist          },
        { "albumartistsort",            'soaa'                     },
        { "albumartistsortorder",       'soaa'                     },
        { "albumsort",                  'soal'                     },
        { "albumsortorder",             'soal'                     },
        { "artist",                     Tag::kArtist               },
        { "artistsort",                 'soar'                     },
        { "artistsortorder",            'soar'                     },
        { "band",                       Tag::kAlbumArtist          },
        { "bpm",                        Tag::kTempo                },
        { "comment",                    Tag::kComment              },
        { "comments",                   Tag::kComment              },
        { "compilation",                Tag::kCompilation          },
        { "composer",                   Tag::kComposer             },
        { "composersort",               'soco'                     },
        { "composersortorder",          'soco'                     },
        { "contentgroup",               Tag::kGrouping             },
        { "copyright",                  Tag::kCopyright            },
        { "date",                       Tag::kDate                 },
        { "description",                'desc'                     },
        { "disc",                       Tag::kDisk                 },
        { "discnumber",                 Tag::kDisk                 },
        { "disctotal",                  TAG_TOTAL_DISCS            },
        { "encodedby",                  FOURCC('\xa9','e','n','c') },
        { "encodingapplication",        Tag::kTool                 },
        { "genre",                      Tag::kGenre                },
        { "grouping",                   Tag::kGrouping             },
        { "itunes:akid",                'akID'                     },
        { "itunes:apid",                'apID'                     },
        { "itunes:atid",                'atID'                     },
        { "itunes:catg",                'catg'                     },
        { "itunes:cmid",                'cmID'                     },
        { "itunes:cnid",                'cnID'                     },
        { "itunes:cpil",                Tag::kCompilation          },
        { "itunes:geid",                'gnID'                     },
        { "itunes:hdvd",                'hdvd'                     },
        { "itunes:keyw",                'keyw'                     },
        { "itunes:pcst",                'pcst'                     },
        { "itunes:pgap",                'pgap'                     },
        { "itunes:plid",                'plID'                     },
        { "itunes:purd",                'purd'                     },
        { "itunes:purl",                'purl'                     },
        { "itunes:rtng",                'rtng'                     },
        { "itunes:sfid",                'sfID'                     },
        { "itunes:sosn",                'sosn'                     },
        { "itunes:stik",                'stik'                     },
        { "itunes:tvep",                'tvep'                     },
        { "itunes:tvnn",                'tvnn'                     },
        { "itunes:tvsh",                'tvsh'                     },
        { "itunes:tvsn",                'tvsn'                     },
        { "itunes:xid",                 'xid '                     },
        { "itunescompilation",          Tag::kCompilation          },
        { "longdescription",            'ldes'                     },
        { "lyrics",                     Tag::kLyrics               },
        { "recordeddate",               Tag::kDate                 },
        { "tempo",                      Tag::kTempo                },
        { "title",                      Tag::kTitle                },
        { "titlesort",                  'sonm'                     },
        { "titlesortorder",             'sonm'                     },
        { "totaldiscs",                 TAG_TOTAL_DISCS            },
        { "totaltracks",                TAG_TOTAL_TRACKS           },
        { "track",                      Tag::kTrack                },
        { "tracknumber",                Tag::kTrack                },
        { "tracktotal",                 TAG_TOTAL_TRACKS           },
        { "unsyncedlyrics",             Tag::kLyrics               },
        { "year",                       Tag::kDate                 },
    };
    uint32_t getFourCCFromTagName(const char *name)
    {
        std::string sname;
        for (const char *s = name; *s; ++s) {
            unsigned char c = *s;
            if (c != ' ' && c != '-' && c != '_')
                sname.push_back(tolower(c));
        }
        name2fcc_t search = { sname.c_str(), 0 };
        auto end = iTunes_name2fcc_map +
            util::sizeof_array(iTunes_name2fcc_map);
        auto entry =
            std::lower_bound(iTunes_name2fcc_map, end, search,
                             [](const name2fcc_t &a,
                                const name2fcc_t &b) -> int
                             { return std::strcmp(a.name, b.name) < 0; });
        return entry < end && !std::strcmp(entry->name, search.name)
                ? entry->fcc : 0;
    }

    void putNumberPair(std::map<uint32_t, std::string> *result,
                       uint32_t fcc, unsigned number, unsigned total)
    {
        if (number) {
            if (total)
                (*result)[fcc] = strutil::format("%u/%u", number, total);
            else
                (*result)[fcc] = strutil::format("%u", number);
        }
    }
    void convertToM4ATags(const std::map<std::string, std::string> &src,
                          std::map<uint32_t, std::string> *shortTags,
                          std::map<std::string, std::string> *longTags)
    {
        std::map<uint32_t, std::string> shortTags_;
        std::map<std::string, std::string> longTags_;
        uint32_t id;
        unsigned disc = 0, track = 0, disc_total = 0, track_total = 0;

        std::for_each(src.begin(), src.end(), [&](decltype(*src.begin()) tag) {
            if ((id = getFourCCFromTagName(tag.first.c_str())) == 0)
                longTags_[tag.first] = tag.second;
            else {
                const char *val = tag.second.c_str();
                switch (id) {
                case TAG_TOTAL_DISCS:
                    std::sscanf(val, "%u", &disc_total); break;
                case TAG_TOTAL_TRACKS:
                    std::sscanf(val, "%u", &track_total); break;
                case Tag::kDisk:
                    std::sscanf(val, "%u/%u", &disc, &disc_total); break;
                case Tag::kTrack:
                    std::sscanf(val, "%u/%u", &track, &track_total); break;
                default:
                    shortTags_[id] = tag.second;
                }
            }
        });
        putNumberPair(&shortTags_, Tag::kTrack, track, track_total);
        putNumberPair(&shortTags_, Tag::kDisk,  disc, disc_total);
        shortTags->swap(shortTags_);
        longTags->swap(longTags_);
    }
    const char *getTagNameFromFourCC(uint32_t fcc)
    {
        fcc2name_t search = { fcc, 0 };
        auto end = iTunes_fcc2name_map +
            util::sizeof_array(iTunes_fcc2name_map);
        auto entry =
            std::lower_bound(iTunes_fcc2name_map, end, search,
                             [](const fcc2name_t &a,
                                const fcc2name_t &b) -> int
                             { return a.fcc < b.fcc; });
        return entry < end && entry->fcc == search.fcc ? entry->name : 0;
    }

    std::string parseValue(uint32_t fcc, const MP4ItmfData &data)
    {
        uint8_t *value = data.value;

        if (fcc == Tag::kGenreID3) {
            unsigned v = (value[0] << 8) | value[1];
            if (v > 0 && v < 255) {
                auto x = static_cast<mp4v2::impl::itmf::GenreType>(v);
                return mp4v2::impl::itmf::enumGenreType.toString(x);
            }
        } else if (fcc == Tag::kDisk || fcc == Tag::kTrack) {
            unsigned index = (value[2] << 8) | value[3];
            unsigned total = (value[4] << 8) | value[5];
            return strutil::format("%u/%u", index, total);
        } else if (data.typeCode == MP4_ITMF_BT_INTEGER) {
            if (data.valueSize == 8) {
                uint32_t high, low;
                high = (value[0]<<24)|(value[1]<<16)|(value[2]<<8)|value[3];
                low  = (value[4]<<24)|(value[5]<<16)|(value[6]<<8)|value[7];
                uint64_t value = (static_cast<uint64_t>(high) << 32) | low;
                return strutil::format("%lld", value);
            }
            int v;
            if (data.valueSize == 1)
                v = value[0];
            else if (data.valueSize == 2)
                v = (value[0] << 8) | value[1];
            else if (data.valueSize == 4)
                v = (value[0]<<24)|(value[1]<<16)|(value[2]<<8)|value[3];
            else
                return "";
            return strutil::format("%d", v);
        } else if (data.typeCode == MP4_ITMF_BT_UTF8) {
            char *vp = reinterpret_cast<char*>(value);
            return std::string(vp, vp + data.valueSize);
        }
        return "";
    }

    void fetchTags(MP4FileX &file, std::map<std::string, std::string> *tags)
    {
        std::map<std::string, std::string> result;
        try {
            auto iL = mp4v2::impl::itmf::genericGetItems(file);
            if (!iL) return;
            std::shared_ptr<MP4ItmfItemList>
                _(iL, mp4v2::impl::itmf::genericItemListFree);
            for (size_t i = 0; i < iL->size; ++i) {
                auto item = iL->elements[i];
                uint32_t fcc = util::fourcc(item.code);
                auto data = item.dataList.elements[0];
                if (!data.value || !data.valueSize)
                    continue;
                auto v = parseValue(fcc, data);
                if (v.empty())
                    continue;
                if (fcc == '----')
                    result[item.name] = v;
                else {
                    const char *name = getTagNameFromFourCC(fcc);
                    if (name) result[name] = v;
                }
            }
            tags->swap(result);
        } catch (mp4v2::impl::Exception *e) {
            handle_mp4error(e);
        }
    }
}

namespace CAF {
    uint64_t next_chunk(int fd, char *name)
    {
        uint64_t size;
        if (util::nread(fd, name, 4) != 4 || util::nread(fd, &size, 8) != 8)
            return 0;
        return util::b2host64(size);
    }
    bool get_info(int fd, std::vector<uint8_t> *info)
    {
        util::FilePositionSaver _(fd);
        if (_lseeki64(fd, 8, SEEK_SET) != 8)
            return false;
        uint64_t chunk_size;
        char chunk_name[4];
        while ((chunk_size = next_chunk(fd, chunk_name)) > 0) {
            if (std::memcmp(chunk_name, "info", 4)) {
                if (_lseeki64(fd, chunk_size, SEEK_CUR) < 0)
                    break;
            } else {
                std::vector<uint8_t> buf(chunk_size);
                if (util::nread(fd, &buf[0], buf.size()) != buf.size())
                    break;
                info->swap(buf);
                return true;
            }
        }
        return false;
    }
    void fetchTags(const std::vector<uint8_t> &info,
                   std::map<std::string, std::string> *dict)
    {
        if (info.size() < 5)
            return;
        // inside of info tag is delimited with NUL char.
        std::map<std::string, std::string> result;
        std::vector<std::string> tokens;
        {
            const char *infop = reinterpret_cast<const char*>(info.data()) + 4;
            const char *endp  = infop + info.size() - 4;
            do {
                tokens.push_back(std::string(infop));
                infop += tokens.back().size() + 1;
            } while (infop < endp);
        }
        for (size_t i = 0; i < tokens.size() >> 1; ++i)
            result[tokens[2 * i]] = tokens[2 * i + 1];
        TextBasedTag::normalizeTags(result, dict);
    }
    bool fetchTags(int fd, std::map<std::string, std::string> *dict)
    {
        std::vector<uint8_t> info;
        if (!get_info(fd, &info) || info.size() < 4)
            return false;
        fetchTags(info, dict);
        return true;
    }
}
