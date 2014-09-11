#ifndef _SINK_H
#define _SINK_H

#include "CoreAudioToolbox.h"
#include "mp4v2wrapper.h"
#include "chapters.h"
#include "iointer.h"
#include "win32util.h"

class MP4SinkBase: public ITagStore {
protected:
    std::wstring m_filename;
    MP4FileX m_mp4file;
    MP4TrackId m_track_id;
    bool m_closed;
    uint32_t m_edit_start;
    uint64_t m_edit_duration;
    std::map<std::string, std::string> m_tags;
    std::vector<chapters::entry_t> m_chapters;
    std::vector<std::vector<char> > m_artworks;
public:
    MP4SinkBase(const std::wstring &path, bool temp=false);
    virtual ~MP4SinkBase() {}

    MP4FileX *getFile() { return &m_mp4file; }
    /* Don't automatically close, since close() involves finalizing */
    void close();
    void setTag(const std::string &key, const std::string &value)
    {
        m_tags[key] = value;
    }
    void setChapters(const std::vector<chapters::entry_t> &chapters)
    {
        m_chapters = chapters;
    }
    void addArtwork(const std::vector<char> &data)
    {
        m_artworks.push_back(data);
    }
    virtual void writeTags();
private:
    void writeShortTag(uint32_t fcc, const std::string &value);
    void writeLongTag(const std::string &key, const std::string &value);

    void writeTrackTag(const char *fcc, const std::string &value);
    void writeDiskTag(const char *fcc, const std::string &value);
    void writeGenreTag(const char *fcc, const std::string &value);
    void writeMediaTypeTag(const char *fcc, const std::string &value);
    void writeRatingTag(const char *fcc, const std::string &value);
    void writeAccountTypeTag(const char *fcc, const std::string &value);
    void writeCountryCodeTag(const char *fcc, const std::string &value);
    void writeInt8Tag(const char *fcc, const std::string &value);
    void writeInt16Tag(const char *fcc, const std::string &value);
    void writeInt32Tag(const char *fcc, const std::string &value);
    void writeInt64Tag(const char *fcc, const std::string &value);
    void writeStringTag(const char *fcc, const std::string &value);
};

class MP4Sink: public ISink, public MP4SinkBase {
    uint32_t m_sample_id;
    AudioFilePacketTableInfo m_priming_info;
    int m_gapless_mode;
public:
    enum {
        MODE_ITUNSMPB = 1,
        MODE_EDTS = 2,
        MODE_BOTH = 3,
    };
    MP4Sink(const std::wstring &path, const std::vector<uint8_t> &cookie,
            bool temp=false);
    void writeSamples(const void *data, size_t length, size_t nsamples)
    {
        try {
            m_mp4file.WriteSample(m_track_id, (const uint8_t *)data,
                                  length, MP4_INVALID_DURATION);
        } catch (mp4v2::impl::Exception *e) {
            handle_mp4error(e);
        }
    }
    void setGaplessInfo(const AudioFilePacketTableInfo &info)
    {
        m_edit_start = info.mPrimingFrames;
        m_edit_duration = info.mNumberValidFrames;
    }
    void setGaplessMode(int mode)
    {
        m_gapless_mode = mode;
    }
    void writeTags();
};

class ALACSink: public ISink, public MP4SinkBase {
public:
    ALACSink(const std::wstring &path, const std::vector<uint8_t> &magicCookie,
             bool temp=false);
    void writeSamples(const void *data, size_t length, size_t nsamples)
    {
        try {
            m_mp4file.WriteSample(m_track_id, (const uint8_t *)data,
                                  length, nsamples);
        } catch (mp4v2::impl::Exception *e) {
            handle_mp4error(e);
        }
    }
};

class ADTSSink: public ISink {
    typedef std::shared_ptr<FILE> file_ptr_t;
    file_ptr_t m_fp;
    uint32_t m_sample_rate_index;
    uint32_t m_channel_config;
    bool m_seekable;
    std::vector<uint8_t> m_pce_data;
public:
    ADTSSink(const std::wstring &path, const std::vector<uint8_t> &cookie);
    ADTSSink(const std::shared_ptr<FILE> &fp,
             const std::vector<uint8_t> &cookie);
    void writeSamples(const void *data, size_t length, size_t nsamples);
private:
    void init(const std::vector<uint8_t> &cookie);
    void write(const void *data, size_t size)
    {
        if (_write(fileno(m_fp.get()), data, size) < 0)
            win32::throw_error("write failed", _doserrno);
    }
};

#endif
