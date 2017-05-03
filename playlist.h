#ifndef PLAYLIST_H
#define PLAYLIST_H

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <stdint.h>
#include "ISource.h"

namespace playlist {
    struct Track {
        unsigned number;
        std::wstring name;
        std::shared_ptr<ISeekableSource> source;
        std::wstring ofilename;
    };
    typedef std::vector<Track> Playlist;

    std::wstring generateFileName(const std::wstring &spec,
                                  const std::map<std::string, std::string>&tag);
}

#endif
