#ifndef _ALACSINK_H
#define _ALACSINK_H

#include "encoderbase.h"
#include "mp4v2wrapper.h"

class ALACSink: public ISink {
    std::wstring m_filename;
    MP4FileX m_mp4file;
    MP4TrackId m_track_id;
    bool m_closed;
public:
    ALACSink(const std::wstring &path, EncoderBase &encoder);
    ~ALACSink() { close(); }
    void writeSamples(const void *data, size_t length, size_t nsamples)
    {
	try {
	    m_mp4file.WriteSample(m_track_id, (const uint8_t *)data,
		length, nsamples);
	} catch (mp4v2::impl::Exception *e) {
	    handle_mp4error(e);
	}
    }
    void close();
};

#endif
