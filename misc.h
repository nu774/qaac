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
    typedef std::complex<float> complex_t;

    std::wstring loadTextFile(const std::wstring &path, uint32_t codepage=0);

    std::wstring generateFileName(const std::wstring &spec,
                                  const std::map<std::string, std::string>&tag);

    std::shared_ptr<FILE> openConfigFile(const wchar_t *file);

    std::vector<std::vector<complex_t>>
    loadRemixerMatrixFromFile(const wchar_t *path);

    std::vector<std::vector<complex_t>>
    loadRemixerMatrixFromPreset(const wchar_t *preset_name);
}

#endif
