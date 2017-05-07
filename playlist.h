#ifndef PLAYLIST_H
#define PLAYLIST_H

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <stdint.h>
#include "ISource.h"

namespace playlist {
    std::wstring generateFileName(const std::wstring &spec,
                                  const std::map<std::string, std::string>&tag);
}

#endif
