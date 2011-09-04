#ifndef _OPTIONS_H
#define _OPTIONS_H

#include <vector>
#include <map>
#include "libsndfilesrc.h"
#include "flacsrc.h"
#include "wvpacksrc.h"
#include "itunetags.h"
#include "resampler.h"

struct Options {
    enum { kABR, kTVBR, kCVBR, kCBR };

    Options() :
	output_format(0),
	method(-1),
	bitrate(-1), quality(2),
	raw_channels(2), raw_sample_rate(44100),
	rate(-1),
	downmix(-1),
	src_mode(10),
	ifilename(0), ofilename(0), outdir(0),
	raw_format(L"S16LE"),
	fname_format(0),
	logfilename(0),
	verbose(true), is_raw(false), is_adts(false), save_stat(false),
       	nice(false), native_chanmapper(false), ignore_length(false),
	no_optimize(false), native_resampler(false)
    {}
    bool parse(int &argc, wchar_t **&argv);

    bool isMP4() const
    {
	return (isAAC() && !is_adts) || isALAC();
    }
    bool isAAC() const
    {
	return output_format == 'aac ' || output_format == 'aach';
    }
    bool isSBR() const
    {
	return output_format == 'aach';
    }
    bool isALAC() const
    {
	return output_format == 'alac';
    }
    bool isLPCM() const
    {
	return output_format == 'lpcm';
    }

    uint32_t output_format;
    int32_t method;
    uint32_t bitrate, quality;
    uint32_t raw_channels, raw_sample_rate;
    int rate; /* -1: keep, 0: auto, others: literal value */
    int downmix; /* -1: none, 1: mono, 2: stereo */
    int src_mode;
    wchar_t *ifilename, *ofilename, *outdir, *raw_format, *fname_format;
    wchar_t *logfilename;
    bool verbose, is_raw, is_adts, save_stat, nice, native_chanmapper,
	 ignore_length, no_optimize, native_resampler;
    std::map<uint32_t, std::wstring> tagopts;
    std::vector<std::wstring> artworks;
    std::wstring encoder_name;
    std::vector<uint32_t> chanmap;

    LibSndfileModule libsndfile;
    FLACModule libflac;
    WavpackModule libwavpack;
    SpeexResamplerModule libspeexdsp;
};

#endif
