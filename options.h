#ifndef _OPTIONS_H
#define _OPTIONS_H

#include <vector>
#include <map>
#include "libsndfilesrc.h"
#include "flacsrc.h"
#include "wvpacksrc.h"
#ifdef _WIN32
#include "taksrc.h"
#endif
#include "itunetags.h"
#include "soxdsp.h"

struct Options {
//    enum { kABR, kTVBR, kCVBR, kCBR };
    enum { kCBR, kABR, kCVBR, kTVBR };

    Options() :
	output_format(0),
	method(-1),
	bitrate(-1), quality(-1),
	raw_channels(2), raw_sample_rate(44100),
	rate(-1),
	lowpass(0),
	chanmask(-1),
	artwork_size(0),
	ifilename(0), ofilename(0), outdir(0),
	raw_format(L"S16LE"),
	fname_format(0),
	chapter_file(0),
	logfilename(0),
	remix_preset(0),
	remix_file(0),
	tmpdir(0),
	verbose(1), is_raw(false), is_adts(false), save_stat(false),
       	nice(false), native_chanmapper(false), ignore_length(false),
	no_optimize(false), native_resampler(false), check_only(false),
	normalize(false), print_available_formats(false), alac_fast(false),
	threading(false),
	textcp(0),
	delay(0),
	gain(0.0),
	is_console_visible(true)
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
    bool isNativeResampler() const
    {
	return !libsoxrate.loaded() || native_resampler;
    }
    const wchar_t *extension() const
    {
	if (isMP4()) return L"m4a";
	else if (isLPCM()) return L"wav";
	else return L"aac";
    }

    uint32_t output_format;
    int32_t method;
    uint32_t bitrate, quality;
    uint32_t raw_channels, raw_sample_rate;
    int rate; /* -1: keep, 0: auto, others: literal value */
    int lowpass;
    int chanmask; /*     -1: honor chanmask in the source(default)
                          0: ignore chanmask in the source
                     others: use the value as chanmask     */
    uint32_t artwork_size;
    wchar_t *ifilename, *ofilename, *outdir, *raw_format, *fname_format;
    wchar_t *chapter_file;
    wchar_t *logfilename;
    wchar_t *remix_preset;
    wchar_t *remix_file;
    wchar_t *tmpdir;
    int verbose;
    bool is_raw, is_adts, save_stat, nice, native_chanmapper,
	 ignore_length, no_optimize, native_resampler, check_only,
	 normalize, print_available_formats, alac_fast, threading;
    uint32_t textcp;
    int delay;
    double gain;
    std::map<uint32_t, std::wstring> tagopts;
    std::vector<std::wstring> artworks;
    std::wstring encoder_name;
    std::vector<uint32_t> chanmap;

    bool is_console_visible;

    LibSndfileModule libsndfile;
    FLACModule libflac;
    WavpackModule libwavpack;
    SoxModule libsoxrate;
#ifdef _WIN32
    TakModule libtak;
#endif
};

#endif
