#ifndef _SINK_H
#define _SINK_H

#include "shared_ptr.h"
#include <CoreAudioTypes.h>
#include "mp4v2wrapper.h"
#include "encoderbase.h"
#include "itunetags.h"

class MP4SinkBase {
protected:
    std::wstring m_filename;
    MP4FileX m_mp4file;
    MP4TrackId m_track_id;
    bool m_closed;
public:
    MP4SinkBase(const std::wstring &path);
    ~MP4SinkBase() { close(); }
    void close();
    void saveTags(TagEditor &editor) { editor.save(m_mp4file); }
    MP4FileX *getFile() { return &m_mp4file; }
};

class MP4Sink: public ISink, public MP4SinkBase {
public:
    MP4Sink(const std::wstring &path, EncoderBase &encoder);
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
    typedef x::shared_ptr<FILE> file_ptr_t;
    file_ptr_t m_fp;
    uint32_t m_sample_rate_index;
    uint32_t m_channel_config;
public:
    ADTSSink(const std::wstring &path, EncoderBase &encoder);
    void writeSamples(const void *data, size_t length, size_t nsamples);
};

#endif
