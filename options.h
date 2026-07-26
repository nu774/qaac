#ifndef _OPTIONS_H
#define _OPTIONS_H

#include <vector>
#include <map>
#include <stdint.h>
#include "misc.h"

struct DRCParams {
    double   m_threshold, m_ratio, m_knee_width, m_attack, m_release;
    const char *m_stat_file;

    DRCParams():
        m_threshold(0.0), m_ratio(0.0), m_knee_width(0.0),
        m_attack(0.0), m_release(0.0),
        m_stat_file(0)
    {}
    DRCParams(double threshold, double ratio, double knee_width,
              double attack, double release, const char *stat_file)
        : m_threshold(threshold), m_ratio(ratio), m_knee_width(knee_width),
          m_attack(attack), m_release(release),
          m_stat_file(stat_file)
    {}
};

struct Options {

//    enum { kABR, kTVBR, kCVBR, kCBR };
    enum { kCBR, kABR, kCVBR, kTVBR };

    Options() :
        method(-1), quality(-1),

        rate(-1), verbose(1), lowpass(0), native_resampler_quality(-1),
        chanmask(-1), num_priming(2112),

        bits_per_sample(0), raw_channels(2), raw_sample_rate(44100),
        artwork_size(0), native_resampler_complexity(0), textcp(0),
        gapless_mode(0),

        ofilename(0), outdir(0), raw_format("S16LE"),
        fname_format("${tracknumber}${title& }${title}"),
        chapter_file(0), logfilename(0), remix_preset(0), remix_file(0),
        tmpdir(0), start(0), end(0), delay(0),

        is_raw(false), is_adts(false), is_caf(false),
        save_stat(false), nice(false), native_chanmapper(false),
        ignore_length(false), no_optimize(false), native_resampler(false),
        check_only(false), normalize(false),
        print_available_formats(false), alac_fast(false), threading(false),
        concat(false), no_matrix_normalize(false), no_dither(false),
        filename_from_tag(false), sort_args(false),
        no_smart_padding(false), limiter(false), copy_artwork(false),

        bitrate(-1.0), gain(0.0),

        output_format(0)
    {}
    bool parse(int &argc, char **&argv);

    bool isMP4() const
    {
        return !is_caf && ((isAAC() && !is_adts) || isALAC());
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
    bool isWaveOut() const
    {
        return output_format == 'play';
    }
    bool isPeak() const
    {
        return output_format == 'peak';
    }
    const char *extension() const
    {
        if (is_caf) return ".caf";
        else if (isMP4()) return ".m4a";
        else if (isLPCM()) return ".wav";
        else if (isWaveOut() || isPeak()) return "";
        else return ".aac";
    }

    int32_t method, quality;
    int rate; /* -1: keep, 0: auto, others: literal value */
    int verbose, lowpass, native_resampler_quality;
    int chanmask; /*     -1: honor chanmask in the source(default)
                          0: ignore chanmask in the source
                     others: use the value as chanmask     */
    unsigned num_priming;
    uint32_t bits_per_sample, raw_channels, raw_sample_rate,
             artwork_size, native_resampler_complexity, textcp,
             gapless_mode;
    const char
            *ofilename, *outdir, *raw_format, *fname_format, *chapter_file,
            *logfilename, *remix_preset, *remix_file, *tmpdir,
            *start, *end, *delay;
    bool is_raw, is_adts, is_caf, save_stat, nice, native_chanmapper,
         ignore_length, no_optimize, native_resampler, check_only,
         normalize, print_available_formats, alac_fast, threading,
         concat, no_matrix_normalize, no_dither, filename_from_tag,
         sort_args, no_smart_padding, limiter, copy_artwork;
    double bitrate, gain;

    uint32_t output_format;
    std::vector<DRCParams> drc_params;
    std::map<uint32_t, std::string> tagopts;
    std::map<uint32_t, std::string> ftagopts;
    std::map<std::string, std::string> longtags;
    std::vector<misc::chapter_t> chapters;
    std::vector<std::string> artwork_files;
    std::vector<std::vector<char> > artworks;
    std::string encoder_name;
    std::vector<uint32_t> chanmap;
    std::vector<int> cue_tracks;
};

#endif
