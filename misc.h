#ifndef MISC_H
#define MISC_H

#include <stdint.h>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>
#include <map>
#include <complex>

namespace misc {
    typedef std::pair<std::string, double> chapter_t;
    typedef std::complex<float> complex_t;

    std::string loadTextFile(const std::string &path, int codepage=0);

    std::string generateFileName(const std::string &spec,
                                 const std::map<std::string, std::string>&tag);

    // the resulting chapter_t is <name, absolute timestamp>
    std::vector<chapter_t> loadChapterFile(const char *file,
                                           uint32_t codepage);

    // convert <name, absolute timestamp> to <name, timedelta>
    std::vector<chapter_t>
        convertChaptersToQT(const std::vector<chapter_t> &chapters,
                            double total_duration);

    std::shared_ptr<FILE> openConfigFile(const char *file);

    std::vector<std::vector<complex_t>>
    loadRemixerMatrixFromFile(const char *path);

    std::vector<std::vector<complex_t>>
    loadRemixerMatrixFromPreset(const char *preset_name);
}

#endif
