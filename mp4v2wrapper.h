#ifndef MP4V2WRAPPER_H
#define MP4V2WRAPPER_H

#include <string>
#include <stdexcept>
#include <GNUCompatibility/stdint.h> // To avoid conflict with QT
#undef FindAtom
#include "impl.h"
#include "util.h"

std::string format_mp4error(const mp4v2::impl::Exception &e);

inline void handle_mp4error(mp4v2::impl::Exception *e)
{
    std::runtime_error re(format_mp4error(*e));
    delete e;
    throw re;
}

class MP4FileCopy;

class MP4FileX: public mp4v2::impl::MP4File {
    friend class MP4FileCopy;
public:
    MP4FileX() {}

    void ResetFile() { m_file = 0; }

    void CreateTemp(const char *prefix,
	    uint32_t flags, int add_ftyp, int add_iods,
	    char *majorBrand, uint32_t minorVersion,
	    char **supportedBrands, uint32_t supportedBrandsCount);

    void FinishWriteX()
    {
	for (size_t i = 0; i < m_pTracks.Size(); ++i)
	    m_pTracks[i]->FinishWrite(0);
    }

    AutoDynaCast<mp4v2::impl::MP4Atom>
    FindAtomT(const char *name)
    {
	return FindAtom(name);
    }
    AutoDynaCast<mp4v2::impl::MP4Atom>
    FindChildAtomT(mp4v2::impl::MP4Atom *parent, const char *name)
    {
	return parent->FindChildAtom(name);
    }
    AutoDynaCast<mp4v2::impl::MP4Atom>
    AddChildAtomT(mp4v2::impl::MP4Atom *parent, const char *name)
    {
	return AddChildAtom(parent, name);
    }
    MP4TrackId AddAlacAudioTrack(const uint8_t *alac, const uint8_t *chan);
    bool SetMetadataString(const char *atom, const char *value);
    bool SetMetadataTrack(uint16_t track, uint16_t totalTracks);
    bool SetMetadataDisk(uint16_t disk, uint16_t totalDisks);
    bool SetMetadataUint8(const char *atom, uint8_t value);
    bool SetMetadataUint16(const char *atom, uint16_t value);
    bool SetMetadataGenre(const char *atom, uint16_t value);
    bool SetMetadataArtwork(const char *atom,
	    const char *data, size_t size,
	    mp4v2::impl::itmf::BasicType typeCode
	     = mp4v2::impl::itmf::BT_UNDEFINED);
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

class MP4FileCopy {
    struct ChunkInfo {
	mp4v2::impl::MP4ChunkId current, final;
	MP4Timestamp time;
    };
    MP4FileX *m_mp4file;
    uint64_t m_nchunks;
    std::vector<ChunkInfo> m_state;
    mp4v2::platform::io::File *m_src;
    mp4v2::platform::io::File *m_dst;
public:
    MP4FileCopy(mp4v2::impl::MP4File *file);
    ~MP4FileCopy() { if (m_dst) finish(); }
    void start(const char *path);
    void finish();
    bool copyNextChunk();
    uint64_t getTotalChunks() { return m_nchunks; }
};
#endif
