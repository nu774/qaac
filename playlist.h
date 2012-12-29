#ifndef PLAYLIST_H
#define PLAYLIST_H

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <stdint.h>
#include "iointer.h"

namespace playlist {
    struct Track {
        std::wstring name;
        std::shared_ptr<ISeekableSource> source;
        std::wstring ofilename;
    };
    typedef std::vector<Track> Playlist;

    std::wstring generateFileName(const std::wstring &spec,
                                  const std::map<uint32_t, std::wstring> &tag);
}

#endif
