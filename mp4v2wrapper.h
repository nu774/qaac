#ifndef MP4V2WRAPPER_H
#define MP4V2WRAPPER_H

#include <string>
#include <stdexcept>
#include <stdint.h>
#undef FindAtom
#include "src/impl.h"
#include "util.h"
#include "chapters.h"

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

    util::AutoDynaCast<mp4v2::impl::MP4Atom>
    FindAtomT(const char *name)
    {
        return FindAtom(name);
    }
    util::AutoDynaCast<mp4v2::impl::MP4Atom>
    FindChildAtomT(mp4v2::impl::MP4Atom *parent, const char *name)
    {
        return parent->FindChildAtom(name);
    }
    util::AutoDynaCast<mp4v2::impl::MP4Atom>
    AddChildAtomT(mp4v2::impl::MP4Atom *parent, const char *name)
    {
        return AddChildAtom(parent, name);
    }
    MP4TrackId AddAlacAudioTrack(const uint8_t *alac, const uint8_t *chan);
    void CreateAudioSampleGroupDescription(MP4TrackId trackId,
                                           uint32_t sampleCount);
    bool SetMetadataString(const char *atom, const char *value);
    bool SetMetadataTrack(uint16_t track, uint16_t totalTracks);
    bool SetMetadataDisk(uint16_t disk, uint16_t totalDisks);
    bool SetMetadataUint8(const char *atom, uint8_t value);
    bool SetMetadataUint16(const char *atom, uint16_t value);
    bool SetMetadataUint32(const char *atom, uint32_t value);
    bool SetMetadataUint64(const char *atom, uint64_t value);
    bool SetMetadataGenre(const char *atom, uint16_t value);
    bool SetMetadataArtwork(const char *atom,
            const char *data, size_t size,
            mp4v2::impl::itmf::BasicType typeCode
             = mp4v2::impl::itmf::BT_UNDEFINED);
    bool SetMetadataFreeForm(const char *name, const char *mean,
              const uint8_t* pValue, uint32_t valueSize,
              mp4v2::impl::itmf::BasicType typeCode
               =mp4v2::impl::itmf::BT_UTF8);
    bool MP4FileX::GetQTChapters(std::vector<chapters::entry_t> *chapters);
    bool MP4FileX::GetNeroChapters(std::vector<chapters::entry_t> *chapters);
    bool MP4FileX::GetChapters(std::vector<chapters::entry_t> *chapters);
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

struct MP4FDReadProvider: public MP4FileProvider
{
    MP4FDReadProvider()
    {
        static MP4FileProvider t = {
            open, seek, read, 0, close
        };
        std::memset(this, 0, sizeof t);
        std::memcpy(this, &t, sizeof t);
    }

    static void *open(const char *name, MP4FileMode mode)
    {
        /*
         * file descriptor (in the form of text string) is get passed as
         * "name". We decode it to an integer, and use it as a handle.
         * Since returning zero is treated as error on mp4v2 side,
         * we cannot use fd 0 (stdin) as handle. So we add 1 to it here,
         * and substruct by 1 on the succeeding jobs.
         */
        int fd = std::strtol(name, 0, 10);
        return reinterpret_cast<void*>(fd + 1);
    }
    static int seek(void *handle, int64_t pos)
    {
        int fd = reinterpret_cast<int>(handle) - 1;
        return _lseeki64(fd, pos, SEEK_SET) < 0;
    }
    static int read(void *handle, void *buffer, int64_t size, int64_t *nin,
                    int64_t maxChunkSize)
    {
        int fd = reinterpret_cast<int>(handle) - 1;
        *nin = util::nread(fd, buffer, size);
        return *nin < 0;
    }
    static int close(void *handle)
    {
        return 0;
    }
};

#endif
