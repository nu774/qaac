#ifndef CONFIG_FILE_H
#define CONFIG_FILE_H

#include <cstdio>
#include <memory>
#include <vector>
#include "MatrixMixer.h"

std::shared_ptr<FILE> openConfigFile(const wchar_t *file);

void loadRemixerMatrixFromFile(const wchar_t *path,
                               std::vector<std::vector<complex_t> > *result);

void loadRemixerMatrixFromPreset(const wchar_t *preset_name,
                                 std::vector<std::vector<complex_t> > *result);

#endif
