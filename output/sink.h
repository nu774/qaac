#ifndef _SINK_H
#define _SINK_H

#include <functional>
#ifdef __APPLE__
#include <CoreServices/CoreServices.h>
#include <AudioToolbox/AudioToolbox.h>
#else
#include "CoreAudio/AudioFile.h"
#endif
#include "mp4v2wrapper.h"
#include "ISink.h"
#include "platformutil.h"
#include "misc.h"

class MP4SinkBase: public ITagStore, public IChapterWriter, public IArtworkWriter, public IFinishWriteSink, public IBitrateWriter {
protected:
    std::string m_filename;
    // Final destination path. Equal to m_filename unless constructed with
    // temp=true, in which case m_filename is overwritten with a scratch
    // filename and finishWrite() later multiplexes into m_dest_filename.
    std::string m_dest_filename;
    bool m_optimize;
    std::function<void(uint64_t, uint64_t)> m_optimize_cb;
    std::shared_ptr<FILE> m_fp;
    MP4FileX m_mp4file;
    MP4TrackId m_track_id;
    bool m_closed;
    uint32_t m_edit_start;
    uint64_t m_edit_duration;
    std::map<std::string, std::string> m_tags;
    std::vector<misc::chapter_t> m_chapters;
    std::vector<std::vector<char> > m_artworks;
    unsigned m_max_bitrate;
public:
    MP4SinkBase(const std::string &path, bool temp);
    virtual ~MP4SinkBase() {}

    MP4FileX *getFile() { return &m_mp4file; }
    /* Don't automatically close, since close() involves finalizing */
    void close();
    void updateMaxBitrate(bool finilize=false);
    void writeBitrates(int avgBitrate=0);
    void setTag(const std::string &key, const std::string &value)
    {
        m_tags[key] = value;
    }
    void setChapters(const std::vector<misc::chapter_t> &chapters)
    {
        m_chapters = chapters;
    }
    void addArtwork(const std::vector<char> &data)
    {
        m_artworks.push_back(data);
    }
    void setOptimizeCallback(std::function<void(uint64_t, uint64_t)> optimize_cb)
    {
        m_optimize_cb = optimize_cb;
    }
    void finishWrite(const AudioFilePacketTableInfo &info);
protected:
    virtual void writeTags();
private:
    void optimize();
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
    AudioFilePacketTableInfo m_priming_info;
    int m_gapless_mode;
public:
    enum {
        MODE_ITUNSMPB = 1,
        MODE_EDTS = 2,
        MODE_BOTH = 3,
    };
    MP4Sink(const std::string &path, const std::vector<uint8_t> &cookie,
            bool temp, int gapless_mode);
    void writeSamples(const void *data, size_t length, size_t nsamples)
    {
        try {
            m_mp4file.WriteSample(m_track_id, (const uint8_t *)data,
                                  length, MP4_INVALID_DURATION);
            updateMaxBitrate();
        } catch (mp4v2::impl::Exception *e) {
            handle_mp4error(e);
        }
    }
protected:
    void writeTags();
};

class ALACSink: public ISink, public MP4SinkBase {
public:
    ALACSink(const std::string &path, const std::vector<uint8_t> &magicCookie,
             bool temp);
    void writeSamples(const void *data, size_t length, size_t nsamples)
    {
        try {
            m_mp4file.WriteSample(m_track_id, (const uint8_t *)data,
                                  length, nsamples);
            updateMaxBitrate();
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
    ADTSSink(const std::string &path, const std::vector<uint8_t> &cookie,
             bool append=false);
    ADTSSink(const std::shared_ptr<FILE> &fp,
             const std::vector<uint8_t> &cookie);
    void writeSamples(const void *data, size_t length, size_t nsamples);
private:
    void init(const std::vector<uint8_t> &cookie);
    void write(const void *data, size_t size)
    {
        if (::write(fileno(m_fp.get()), data, size) < 0)
            util::throw_crt_error("write failed");
    }
};

#endif
