#include <array>
#include <algorithm>
#include <cstdlib>
#include <riff/aiff/aifffile.h>
#include <ape/apefile.h>
#include <ape/apetag.h>
#include <mpeg/mpegfile.h>
#include <mpeg/id3v1/id3v1genres.h>
#include <mpeg/id3v2/id3v2framefactory.h>
#include <mpeg/id3v2/frames/textidentificationframe.h>
#include <mpeg/id3v2/frames/attachedpictureframe.h>
#include "metadata.h"
#include "misc.h"
#ifdef _WIN32
#include "platformutil.h"
#endif
#include "strutil.h"
#include "mp4v2wrapper.h"
#include "cuesheet.h"
#include "taglibhelper.h"

namespace {
    typedef const char *kvpair_t[2];

    const char* lookup_by_key(const kvpair_t* begin, const kvpair_t *end,
                              const char *key)
    {
        kvpair_t search = { key, 0 };
        auto entry =
            std::lower_bound(begin, end, search,
                             [](const kvpair_t &a, const kvpair_t &b) -> bool {
                                return std::strcmp(a[0], b[0]) < 0;
                             });
        if (entry < end && !std::strcmp((*entry)[0], key))
            return (*entry)[1];
        else
            return 0;
    }
}

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
        std::string sname =
            strutil::slower(strutil::squeeze(std::string(name), " -_"));
        auto end = known_keys + util::sizeof_array(known_keys);
        auto found = lookup_by_key(known_keys, end, sname.c_str());
        return found ? found : strutil::supper(name);
    }
    std::map<std::string, std::string>
        normalizeTags(const std::map<std::string, std::string> &src)
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
        return result;
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
    std::map<std::string, std::string> fetchID3v2Tags(TagLib::ID3v2::Tag *tag)
    {
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
                auto k = fields.begin()->to8Bit(true);
                auto v = (++fields.begin())->to8Bit(true);
                tags[k] = v;
            } else if (sID == "TCON") {
                tags["genre"] = tag->genre().to8Bit(true);
            } else if (sID == "APIC") {
                auto picframe =
                    dynamic_cast<TagLib::ID3v2::AttachedPictureFrame*>(frame);
                if (picframe->type() ==
                    TagLib::ID3v2::AttachedPictureFrame::FrontCover)
                {
                    auto pic = picframe->picture();
                    tags["COVER ART"] = std::string(pic.begin(), pic.end());
                }
            } else {
                auto end = known_keys + util::sizeof_array(known_keys);
                auto key = lookup_by_key(known_keys, end, sID.c_str());
                if (key)
                    tags[key] = frame->toString().to8Bit(true);
            }
        });
        return TextBasedTag::normalizeTags(tags);
    }
    std::map<std::string, std::string> fetchAiffID3Tags(std::shared_ptr<IInputStream> stream)
    {
        util::FilePositionSaver _(stream);
        stream->seek(0, SEEK_SET);
        TagLibX::IStreamReader reader(stream);
        TagLib::RIFF::AIFF::File file(&reader, false);
        auto tag = file.tag();
        return fetchID3v2Tags(tag);
    }
    std::map<std::string, std::string> fetchMPEGID3Tags(std::shared_ptr<IInputStream> stream)
    {
        util::FilePositionSaver _(stream);
        stream->seek(0, SEEK_SET);
        TagLibX::IStreamReader reader(stream);
        TagLib::MPEG::File file(&reader,
                                TagLib::ID3v2::FrameFactory::instance(),
                                false);
        auto tag = file.ID3v2Tag();
        return fetchID3v2Tags(tag);
    }
}

namespace M4A {
    enum { TAG_TOTAL_DISCS = 1, TAG_TOTAL_TRACKS = 2 };

    struct fcc2name_t {
        uint32_t fcc;
        const char *name;
    };

    struct name2fcc_t {
        const char *name;
        uint32_t fcc;
    };

    struct fcc2type_t {
        uint32_t fcc;
        uint8_t type;
        int length;
    };

    const fcc2name_t iTunes_fcc2name_map[] = {
        { 'aART',                     "ALBUM ARTIST"          }, 
        { 'akID',                     "iTunes:akID"          },
        { 'apID',                     "iTunes:apID"          },
        { 'atID',                     "iTunes:atID"          },
        { 'catg',                     "iTunes:catg"          },
        { 'cmID',                     "iTunes:cmID"          },
        { 'cnID',                     "iTunes:cnID"          },
        { 'covr',                     "COVER ART"            },
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
        { "coverart",                   Tag::kArtwork              },
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
        { "itunes:geid",                'geID'                     },
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

    const fcc2type_t itunes_fcc2type_map[] = {
        { 'akID', ITMF_TYPE_INTEGER, 1 },
        { 'atID', ITMF_TYPE_INTEGER, 4 },
        { 'cmID', ITMF_TYPE_INTEGER, 4 },
        { 'cnID', ITMF_TYPE_INTEGER, 4 },
        { 'cpil', ITMF_TYPE_INTEGER, 1 },
        { 'disk', ITMF_TYPE_IMPLICIT, 6 },
        { 'geID', ITMF_TYPE_INTEGER, 4 },
        { 'gnre', ITMF_TYPE_IMPLICIT, 2 },
        { 'hdvd', ITMF_TYPE_INTEGER, 1 },
        { 'pcst', ITMF_TYPE_INTEGER, 1 },
        { 'pgap', ITMF_TYPE_INTEGER, 1 },
        { 'plID', ITMF_TYPE_INTEGER, 8 },
        { 'purl', ITMF_TYPE_IMPLICIT, 0 },
        { 'rtng', ITMF_TYPE_INTEGER, 1 },
        { 'sfID', ITMF_TYPE_INTEGER, 4 },
        { 'stik', ITMF_TYPE_INTEGER, 1 },
        { 'tmpo', ITMF_TYPE_INTEGER, 2 },
        { 'trkn', ITMF_TYPE_IMPLICIT, 8 },
        { 'tves', ITMF_TYPE_INTEGER, 4 },
        { 'tvsn', ITMF_TYPE_INTEGER, 4 },
    };

    uint32_t getFourCCFromTagName(const char *name)
    {
        std::string sname =
            strutil::slower(strutil::squeeze(std::string(name), " -_"));
        name2fcc_t search = { sname.c_str(), 0 };
        auto end = iTunes_name2fcc_map +
            util::sizeof_array(iTunes_name2fcc_map);
        auto entry =
            std::lower_bound(iTunes_name2fcc_map, end, search,
                             [](const name2fcc_t &a,
                                const name2fcc_t &b) -> bool
                             { return std::strcmp(a.name, b.name) < 0; });
        return entry < end && !std::strcmp(entry->name, search.name)
                ? entry->fcc : 0;
    }

    void putNumberPair(std::map<uint32_t, std::string> *dst,
                       uint32_t fcc, unsigned number, unsigned total)
    {
        if (number) {
            if (total)
                (*dst)[fcc] = strutil::format("%u/%u", number, total);
            else
                (*dst)[fcc] = strutil::format("%u", number);
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
                                const fcc2name_t &b) -> bool
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
        } else {
            char *vp = reinterpret_cast<char*>(value);
            return std::string(vp, vp + data.valueSize);
        }
        return "";
    }

    std::map<std::string, std::string> fetchTags(MP4FileX &file)
    {
        std::map<std::string, std::string> result;
        try {
            auto iL = mp4v2::impl::itmf::genericGetItems(file);
            if (!iL) return result;
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
                    result.insert(std::make_pair(std::string(item.name), v));
                else {
                    const char *name = getTagNameFromFourCC(fcc);
                    if (name)
                        result.insert(std::make_pair(std::string(name), v));
                }
            }
        } catch (mp4v2::impl::Exception *e) {
            handle_mp4error(e);
        }
        return result;
    }

    class UDTAReader {
        const uint8_t *ptr;
        int size;
    public:    
        UDTAReader(const void *p, size_t len) {
            ptr = static_cast<const uint8_t*>(p);
            size = len;
        }
        int remaining() {
            return size;
        }
        UDTAReader sub_reader(size_t len) {
            return UDTAReader(ptr, len);
        }
        uint32_t u32be()
        {
            if (size < 4) return 0;
            uint32_t rc = ptr[0]<<24 | ptr[1]<<16 |ptr[2]<<8 | ptr[3];
            ptr += 4;
            size -= 4;
            return rc;
        }
        uint32_t u64be()
        {
            uint32_t high = u32be();
            uint32_t lo = u32be();
            return static_cast<uint64_t>(high)<<32 | lo;
        }
        uint8_t u8()
        {
            if (size == 0) return 0;
            uint8_t value = *ptr++;
            size--;
            return value;
        }
        uint32_t header(uint32_t *len)
        {
            if (size < 8) {
                *len = 0;
                return 0;
            }
            *len = u32be() - 8;
            return u32be();
        }
        std::string str(int len)
        {
            int n = std::min(len, size);
            std::string res(ptr, ptr + n);
            ptr += n;
            size -= n;
            return res;
        }
        std::string remaining_str()
        {
            std::string res(ptr, ptr + size);
            ptr += size;
            size = 0;
            return res;
        }
        void skip(int len)
        {
            int n = std::min(len, size);
            ptr += n;
            size -= n;
        }
    };

    class UDTAWriter {
        std::vector<uint8_t> buffer;
    public:
        int position() {
            return buffer.size();
        }
        const std::vector<uint8_t> &data() {
            return buffer;
        }
        void write_bytes(const void *data, int len) {
            const uint8_t *p = static_cast<const uint8_t*>(data);
            std::copy(p, p + len, std::back_inserter(buffer));
        }
        void write_bytes_at(int pos, const void *data, int len) {
            const uint8_t *p = static_cast<const uint8_t*>(data);
            if (pos + len > buffer.size()) {
                buffer.resize(pos + len);
            }
            std::copy(p, p + len, &buffer[pos]);
        }
        void write_u32be(uint32_t n) {
            uint8_t data[4] = {
                static_cast<uint8_t>(n>>24),
                static_cast<uint8_t>((n>>16) & 0xff),
                static_cast<uint8_t>((n>>8) & 0xff),
                static_cast<uint8_t>(n & 0xff)
            };
            write_bytes(data, 4);
        }
        void write_u32be_at(int pos, uint32_t n) {
            uint8_t data[4] = {
                static_cast<uint8_t>(n>>24),
                static_cast<uint8_t>((n>>16) & 0xff),
                static_cast<uint8_t>((n>>8) & 0xff),
                static_cast<uint8_t>(n & 0xff)
            };
            write_bytes_at(pos, data, 4);
        }
        void write_u64be(uint64_t n) {
            write_u32be(n >> 32);
            write_u32be(n & 0xffffffff);
        }
        void write_u64be_at(int pos, uint64_t n) {
            write_u32be_at(pos, n >> 32);
            write_u32be_at(pos + 4, n & 0xffffffff);
        }
    };

    class ITMFParser {
        std::vector<ITMFItem> items;
    public:
        ITMFParser(const void *p, size_t len) {
            UDTAReader reader(p, len);
            udta(reader);
        }
        std::vector<ITMFItem> result() {
            return items;
        }
        void udta(UDTAReader &reader) {
            uint32_t size;
            while (true) {
                uint32_t type = reader.header(&size);
                if (!type) break;
                if (type == 'meta') {
                    UDTAReader sub = reader.sub_reader(size);
                    meta(sub);
                }
                reader.skip(size);
            }
        }
        void meta(UDTAReader &reader) {
            reader.skip(4); // version: 0, flags: 0
            uint32_t size;
            while (true) {
                uint32_t type = reader.header(&size);
                if (!type) break;
                if (type == 'ilst') {
                    UDTAReader sub = reader.sub_reader(size);
                    ilst(sub);
                }
                reader.skip(size);
            }
        }
        void ilst(UDTAReader &reader) {
            uint32_t size;
            while (true) {
                uint32_t type = reader.header(&size);
                if (!type) break;
                UDTAReader sub = reader.sub_reader(size);
                ITMFItem item;
                item.code = type;
                this->item(sub, item);
                reader.skip(size);
            }
        }
        void item(UDTAReader &reader, ITMFItem &item) {
            uint32_t size;
            while (true) {
                uint32_t type = reader.header(&size);
                if (!type) break;
                UDTAReader sub = reader.sub_reader(size);
                switch (type) {
                case 'mean':
                    mean(sub, item);
                    break;
                case 'name':
                    name(sub, item);
                    break;
                case 'data':
                    data(sub, item);
                    break;
                }
                reader.skip(size);
            }
            items.push_back(item);
        }
        void mean(UDTAReader &reader, ITMFItem &item) {
            reader.skip(4);
            item.mean = reader.remaining_str();
        }
        void name(UDTAReader &reader, ITMFItem &item) {
            reader.skip(4);
            item.name = reader.remaining_str();
        }
        void data(UDTAReader &reader, ITMFItem &item) {
            auto sub = reader.sub_reader(4);
            type_indicator(sub, item);
            reader.skip(8);
            item.value = reader.remaining_str();
        }
        void type_indicator(UDTAReader &reader, ITMFItem &item) {
            reader.skip(3);
            item.type = reader.u8();
        }
    };

    class NeroChapterParser {
        std::vector<misc::chapter_t> items;
    public:
        NeroChapterParser(const void *p, size_t len) {
            UDTAReader reader(p, len);
            udta(reader);
        }
        std::vector<misc::chapter_t> result() {
            return items;
        }
        void udta(UDTAReader &reader) {
            uint32_t size;
            while (true) {
                uint32_t type = reader.header(&size);
                if (!type) break;
                if (type == 'chpl') {
                    UDTAReader sub = reader.sub_reader(size);
                    chpl(sub);
                }
                reader.skip(size);
            }
        }
        void chpl(UDTAReader &reader) {
            reader.skip(5); // version(u8), flags(u24), reserved(u8)
            uint32_t count = reader.u32be();
            for (uint32_t i = 0; i < count; ++i) {
                uint64_t startTime = reader.u64be();
                uint8_t len = reader.u8();
                std::string title = reader.str(len);
                items.emplace_back(title, startTime / 10000000.0);
            }
        }
    };

    class ITMFSerializer {
        UDTAWriter writer;
        std::vector<ITMFItem> items;
    public:
        ITMFSerializer(const std::vector<ITMFItem> &items) {
            this->items = items;
            write_meta();
        }
        const std::vector<uint8_t> &result() {
            return writer.data();
        }
    private:
        void write_meta() {
            uint32_t position = writer.position();
            writer.write_u32be(0);
            writer.write_bytes("meta", 4);
            writer.write_bytes("\x00\x00\x00\x00", 4); // version + flags
            write_hdlr();
            write_ilst();
            writer.write_u32be_at(position, writer.position() - position);
        }
        void write_hdlr() {
            uint32_t size = 8 + 4 + 4 + 4 + 4 * 3 + 1;
            writer.write_u32be(size);
            writer.write_bytes("hdlr", 4);
            writer.write_bytes("\x00\x00\x00\x00", 4); // version + flags
            writer.write_bytes("\x00\x00\x00\x00", 4); // reserved
            writer.write_bytes("mdir", 4); // handler type
            writer.write_bytes("appl\x00\x00\x00\x00\x00\x00\x00\x00", 12); // reserved
            writer.write_bytes("\x00", 1); // empty name
        }
        void write_ilst() {
            uint32_t position = writer.position();
            writer.write_u32be(0);
            writer.write_bytes("ilst", 4);
            for (auto &&item: items) {
                write_item(item);
            }
            writer.write_u32be_at(position, writer.position() - position);
        }
        void write_item(const ITMFItem& item) {
            uint32_t position = writer.position();
            writer.write_u32be(0);
            writer.write_u32be(item.code);
            if (item.code == '----') {
                write_mean(item);
                write_name(item);
            }
            write_data(item);
            writer.write_u32be_at(position, writer.position() - position);
        }
        void write_mean(const ITMFItem& item) {
            uint32_t size = item.mean.size() + 8 + 4;
            writer.write_u32be(size);
            writer.write_bytes("mean", 4);
            writer.write_bytes("\x00\x00\x00\x00", 4); // version + flags
            writer.write_bytes(item.mean.data(), item.mean.size());
        }
        void write_name(const ITMFItem& item) {
            uint32_t size = item.name.size() + 8 + 4;
            writer.write_u32be(size);
            writer.write_bytes("name", 4);
            writer.write_bytes("\x00\x00\x00\x00", 4); // version + flags
            writer.write_bytes(item.name.data(), item.name.size());
        }
        void write_data(const ITMFItem& item) {
            uint32_t size = item.value.size() + 8 + 8;
            writer.write_u32be(size);
            writer.write_bytes("data", 4);
            writer.write_bytes("\x00\x00", 2); // reserved
            writer.write_bytes("\x00", 1); // type_set_identifier
            writer.write_bytes(&item.type, 1);
            writer.write_bytes("\x00\x00\x00\x00", 4); // the_locale
            writer.write_bytes(item.value.data(), item.value.size());
        }
    };

    class NeroChapterSerializer {
        UDTAWriter writer;
        std::vector<misc::chapter_t> items;
    public:
        NeroChapterSerializer(const std::vector<misc::chapter_t>& items) {
            this->items = items;
            write_chpl();
        }
        const std::vector<uint8_t>& result() {
            return writer.data();
        }
    private:
        void write_chpl() {
            uint32_t position = writer.position();
            writer.write_u32be(0);
            writer.write_bytes("chpl", 4);
            writer.write_bytes("\x01\x00\x00\x00\x00", 5); // version + flags + reserved
            writer.write_u32be(items.size());
            for (auto&& item : items) {
                writer.write_u64be(static_cast<uint64_t>(item.second * 10000000));
                std::string title = item.first;
                uint8_t len = title.size();
                writer.write_bytes(&len, 1);
                writer.write_bytes(title.data(), title.size());
            }
            writer.write_u32be_at(position, writer.position() - position);
        }
    };

    std::vector<ITMFItem> parseUdtaMeta(const void *udta, size_t len)
    {
        return ITMFParser(udta, len).result();
    }

    std::vector<misc::chapter_t> parseUdtaChpl(const void *udta, size_t len)
    {
        return NeroChapterParser(udta, len).result();
    }

    std::string parseValue(const ITMFItem &item)
    {
        uint32_t fcc = item.code;
        auto &&value = item.value;
        if (fcc == Tag::kGenreID3) {
            unsigned v = (value[0] << 8) | value[1];
            if (v > 0 && v < 255) {
                return TagLib::ID3v1::genre(v - 1).to8Bit();
            }
        } else if (fcc == Tag::kDisk || fcc == Tag::kTrack) {
            unsigned index = (value[2] << 8) | value[3];
            unsigned total = (value[4] << 8) | value[5];
            return strutil::format("%u/%u", index, total);
        } else if (item.type == ITMF_TYPE_INTEGER) {
            if (value.size() == 8) {
                uint32_t high, low;
                high = (value[0]<<24)|(value[1]<<16)|(value[2]<<8)|value[3];
                low  = (value[4]<<24)|(value[5]<<16)|(value[6]<<8)|value[7];
                uint64_t value64 = (static_cast<uint64_t>(high) << 32) | low;
                return strutil::format("%lld", value64);
            }
            int v;
            if (value.size() == 1)
                v = value[0];
            else if (value.size() == 2)
                v = (value[0] << 8) | value[1];
            else if (value.size() == 4)
                v = (value[0]<<24)|(value[1]<<16)|(value[2]<<8)|value[3];
            else
                return "";
            return strutil::format("%d", v);
        } else {
            return value;
        }
        return "";
    }

    std::map<std::string, std::string> convertToStringTags(const std::vector<ITMFItem> &tags)
    {
        std::map<std::string, std::string> result;
        for (auto &&item: tags)
        {
            uint32_t fcc = item.code;
            auto v = parseValue(item);
            if (v.empty())
                continue;
            if (fcc == '----')
                result.insert(std::make_pair(std::string(item.name), v));
            else {
                const char *name = getTagNameFromFourCC(fcc);
                if (name)
                    result.insert(std::make_pair(std::string(name), v));
            }
        }
        return result;
    }

    std::vector<uint8_t> serializeUdtaMeta(const std::vector<ITMFItem> &items)
    {
        return ITMFSerializer(items).result();
    }

    std::vector<uint8_t> serializeUdtaChpl(const std::vector<misc::chapter_t>& items)
    {
        return NeroChapterSerializer(items).result();
    }

    uint8_t getTagTypeFromFourCC(uint32_t fcc, unsigned *size)
    {
        fcc2type_t search = { fcc, 0 };
        auto end = itunes_fcc2type_map +
            util::sizeof_array(itunes_fcc2type_map);
        auto entry =
            std::lower_bound(itunes_fcc2type_map, end, search,
                             [](const fcc2type_t &a,
                                const fcc2type_t &b) -> bool
                             { return a.fcc < b.fcc; });
        if (entry < end && entry->fcc == search.fcc) {
            *size = entry->length;
            return entry->type;
        }
        return ITMF_TYPE_UTF8;
    }

    std::vector<ITMFItem> convertToM4aTags(const std::map<std::string, std::string> tags)
    {
        std::vector<ITMFItem> result;
        unsigned track = 0;
        unsigned track_total = 0;
        unsigned disc = 0;
        unsigned disc_total = 0;

        for (auto &&it: tags) {
            ITMFItem item;
            uint32_t fcc = getFourCCFromTagName(it.first.c_str());
            if (fcc == 0) {
                item.code = '----';
                item.mean = "com.apple.iTunes";
                item.name = it.first;
                item.value = it.second;
                if (item.name == "Encoding Params") {
                    item.type = ITMF_TYPE_IMPLICIT;
                } else {
                    item.type = ITMF_TYPE_UTF8;
                }
            } else {
                item.code = fcc;
                unsigned size;
                uint8_t type = getTagTypeFromFourCC(fcc, &size);
                item.type = type;
                if (type == ITMF_TYPE_INTEGER) {
                    try {
                        uint64_t value = std::stoull(it.second);
                        if (size == 1) {
                            std::array<uint8_t, 1> buf{};
                            buf[0] = value;
                            item.value = std::string(buf.begin(), buf.end());
                        } else if (size == 2) {
                            std::array<uint8_t, 2> buf{};
                            buf[0] = value >> 8;
                            buf[1] = value;
                            item.value = std::string(buf.begin(), buf.end());
                        } else if (size == 4) {
                            uint32_t value4 = util::h2big32(static_cast<uint32_t>(value));
                            std::array<uint8_t, 4> buf{};
                            std::memcpy(buf.data(), &value4, 4);
                            item.value = std::string(buf.begin(), buf.end());
                        } else if (size == 8) {
#ifdef _MSC_VER
                            value = _byteswap_uint64(value);
#else
                            value = __builtin_bswap64(value);
#endif
                            std::array<uint8_t, 8> buf{};
                            std::memcpy(buf.data(), &value, 8);
                            item.value = std::string(buf.begin(), buf.end());
                        }
                    } catch (...) {
                        continue;
                    }
                } else if (type == ITMF_TYPE_IMPLICIT) {
                    int genreid;
                    switch (fcc) {
                    case Tag::kDisk:
                        std::sscanf(it.second.c_str(), "%u/%u", &disc, &disc_total);
                        continue;
                    case Tag::kTrack:
                        std::sscanf(it.second.c_str(), "%u/%u", &track, &track_total);
                        continue;
                    case TAG_TOTAL_DISCS:
                        std::sscanf(it.second.c_str(), "%u", &disc_total);
                        continue;
                    case TAG_TOTAL_TRACKS:
                        std::sscanf(it.second.c_str(), "%u", &track_total);
                        continue;
                    case Tag::kGenreID3:
                    case Tag::kGenre:
                        genreid = TagLib::ID3v1::genreIndex(it.second);
                        if (genreid < 255) {
                            std::array<uint8_t, 2> buf{};
                            buf[1] = genreid + 1;
                            item.value = std::string(buf.begin(), buf.end());
                        } else {
                            item.code = Tag::kGenre;
                            item.value = it.second;
                        }
                        break;
                    default:
                        item.value = it.second;
                        break;
                    }
                } else {
                    item.value = it.second;
                }
            }
            result.push_back(item);
        }
        if (track) {
            ITMFItem item;
            item.code = Tag::kTrack;
            item.type = ITMF_TYPE_IMPLICIT;
            std::array<uint8_t, 8> buf{};
            buf[2] = track >> 8;
            buf[3] = track;
            buf[4] = track_total >> 8;
            buf[5] = track_total;
            item.value = std::string(buf.begin(), buf.end());
            result.push_back(item);
        }
        if (disc) {
            ITMFItem item;
            item.code = Tag::kDisk;
            item.type = ITMF_TYPE_IMPLICIT;
            std::array<uint8_t, 6> buf{};
            buf[2] = disc >> 8;
            buf[3] = disc;
            buf[4] = disc_total >> 8;
            buf[5] = disc_total;
            item.value = std::string(buf.begin(), buf.end());
            result.push_back(item);
        }
        return result;
    }
}

namespace CAF {
    uint64_t next_chunk(IInputStream *stream, char *name)
    {
        uint64_t size;
        if (stream->read(name, 4) != 4 || stream->read(&size, 8) != 8)
            return 0;
        return util::b2host64(size);
    }
    std::vector<uint8_t> get_info(std::shared_ptr<IInputStream> stream)
    {
        std::vector<uint8_t> buf;
        util::FilePositionSaver _(stream);
        if (stream->seek(8, SEEK_SET) != 8)
            return buf;
        uint64_t chunk_size;
        char chunk_name[4];
        while ((chunk_size = next_chunk(stream.get(), chunk_name)) > 0) {
            if (std::memcmp(chunk_name, "info", 4)) {
                if (stream->seek(chunk_size, SEEK_CUR) < 0)
                    break;
            } else {
                buf.resize(chunk_size);
                if (stream->read(&buf[0], buf.size()) != buf.size())
                    break;
                return buf;
            }
        }
        return buf;
    }
    std::map<std::string, std::string>
        fetchTags(const std::vector<uint8_t> &info)
    {
        std::map<std::string, std::string> result;
        if (info.size() < 5)
            return result;
        // inside of info tag is delimited with NUL char.
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
        return TextBasedTag::normalizeTags(result);
    }
    std::map<std::string, std::string> fetchTags(std::shared_ptr<IInputStream> stream)
    {
        auto info = get_info(stream);
        if (info.size() < 4)
            return std::map<std::string, std::string>();
        return fetchTags(info);
    }
}
