#ifndef MP4V2WRAPPER_H
#define MP4V2WRAPPER_H

#include <string>
#include <stdexcept>
#include <GNUCompatibility/stdint.h> // To avoid conflict with QT
#undef FindAtom
#include "impl.h"

std::string format_mp4error(const mp4v2::impl::Exception &e);

inline void handle_mp4error(mp4v2::impl::Exception *e)
{
    std::runtime_error re(format_mp4error(*e));
    delete e;
    throw re;
}

class MP4FileX: public mp4v2::impl::MP4File {
public:
    MP4FileX() {}

    MP4TrackId AddAlacAudioTrack(uint32_t timeScale, const uint8_t *cookie,
	    size_t cookieLength);
    bool SetMetadataString(const char *atom, const char *value);
    bool SetMetadataTrack(uint16_t track, uint16_t totalTracks);
    bool SetMetadataDisk(uint16_t disk, uint16_t totalDisks);
    bool SetMetadataUint8(const char *atom, uint8_t value);
    bool SetMetadataUint16(const char *atom, uint16_t value);
    bool SetMetadataGenre(const char *atom, uint16_t value);
    bool SetMetadataFreeForm(const char *name, const char *mean,
	      const uint8_t* pValue, uint32_t valueSize,
	      mp4v2::impl::itmf::BasicType typeCode
	       =mp4v2::impl::itmf::BT_IMPLICIT);
protected:
    mp4v2::impl::MP4DataAtom *CreateMetadataAtom(const char *name,
	    mp4v2::impl::itmf::BasicType typeCode);
    mp4v2::impl::MP4DataAtom *FindOrCreateMetadataAtom(const char *atom,
	    mp4v2::impl::itmf::BasicType typeCode);
};
#endif
