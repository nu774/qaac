#ifndef PLAYLIST_H
#define PLAYLIST_H

namespace playlist {
    struct Track {
        std::wstring name;
        std::shared_ptr<ISeekableSource> source;
        std::wstring ofilename;
    };
    typedef std::vector<Track> Playlist;
}

#endif
