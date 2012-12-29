#ifndef _SINK_H
#define _SINK_H

#include "CoreAudioToolbox.h"
#include "mp4v2wrapper.h"
#include "itunetags.h"
#include "iencoder.h"

class MP4SinkBase {
protected:
    std::wstring m_filename;
    MP4FileX m_mp4file;
    MP4TrackId m_track_id;
    bool m_closed;
public:
    MP4SinkBase(const std::wstring &path, bool temp=false);
    MP4FileX *getFile() { return &m_mp4file; }
    /* Don't automatically close, since close() involves finalizing */
    void close();
};

class MP4Sink: public ISink, public MP4SinkBase {
public:
    MP4Sink(const std::wstring &path, const std::vector<uint8_t> &cookie,
            uint32_t fcc, bool temp=false);
    void writeSamples(const void *data, size_t length, size_t nsamples)
    {
        try {
            m_mp4file.WriteSample(m_track_id, (const uint8_t *)data,
                length, MP4_INVALID_DURATION);
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
public:
    ADTSSink(const std::wstring &path, const std::vector<uint8_t> &cookie);
    ADTSSink(const std::shared_ptr<FILE> &fp,
             const std::vector<uint8_t> &cookie);
    void writeSamples(const void *data, size_t length, size_t nsamples);
private:
    void init(const std::vector<uint8_t> &cookie);
};

#endif
