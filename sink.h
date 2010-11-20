#ifndef _SINK_H
#define _SINK_H

#if _MSC_VER >= 1500
#include <memory>
#else
#include <boost/tr1/memory.hpp>
#endif
#include <CoreAudioTypes.h>
#include <stdint.h>
#include "mp4v2wrapper.h"
#include "encoderbase.h"

class MP4Sink: public ISink {
    std::wstring m_filename;
    mp4v2::impl::MP4File m_mp4file;
    MP4TrackId m_track_id;
    bool m_closed;
public:
    MP4Sink(const std::wstring &path, EncoderBase &encoder);
    ~MP4Sink() { close(); }
    void writeSamples(const void *data, size_t length, size_t nsamples)
    {
	try {
	    m_mp4file.WriteSample(m_track_id, (const uint8_t *)data,
		length, MP4_INVALID_DURATION);
	} catch (mp4v2::impl::MP4Error *e) {
	    handle_mp4error(e);
	}
    }
    void close();
};

class ADTSSink: public ISink {
    typedef std::tr1::shared_ptr<FILE> file_ptr_t;
    file_ptr_t m_fp;
    uint32_t m_sample_rate_index;
    uint32_t m_channel_config;
public:
    ADTSSink(const std::wstring &path, EncoderBase &encoder);
    void writeSamples(const void *data, size_t length, size_t nsamples);
};

#endif
