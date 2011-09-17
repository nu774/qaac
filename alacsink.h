#ifndef _ALACSINK_H
#define _ALACSINK_H

#include "encoderbase.h"
#include "sink.h"

class ALACSink: public ISink, public MP4SinkBase {
public:
    ALACSink(const std::wstring &path, EncoderBase &encoder);
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

#endif
