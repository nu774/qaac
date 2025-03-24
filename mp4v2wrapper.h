#ifndef MP4V2WRAPPER_H
#define MP4V2WRAPPER_H

#include <cstdio>
#include <string>
#include <stdexcept>
#include <stdint.h>
#undef FindAtom
#include "src/impl.h"
#include "util.h"
#include "misc.h"

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

    void FinishWriteX()
    {
        for (size_t i = 0; i < m_pTracks.Size(); ++i)
            m_pTracks[i]->FinishWrite(MP4_CLOSE_DO_NOT_COMPUTE_BITRATE);
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
    bool GetQTChapters(std::vector<misc::chapter_t> *chapters);
    bool GetNeroChapters(std::vector<misc::chapter_t> *chapters,
                         double *first_off);
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
    std::shared_ptr<FILE> m_fp;
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

struct MP4StdIOCallbacks: public MP4IOCallbacks
{
    MP4StdIOCallbacks()
    {
        static MP4IOCallbacks t = {
            get_size, seek, read, write, truncate
        };
        std::memset(this, 0, sizeof t);
        std::memcpy(this, &t, sizeof t);
    }

    static int64_t get_size(void *handle)
    {
        FILE *fp = static_cast<FILE*>(handle);
        fflush(fp);
        return _filelengthi64(_fileno(fp));
    }
    static int seek(void *handle, int64_t pos)
    {
        FILE *fp = static_cast<FILE*>(handle);
        return _fseeki64(fp, pos, SEEK_SET) < 0;
    }
    static int read(void *handle, void *buffer, int64_t size, int64_t *nin)
    {
        FILE *fp = static_cast<FILE*>(handle);
        *nin = fread(buffer, 1, size, fp);
        return ferror(fp);
    }
    static int write(void *handle, const void *buffer, int64_t size, int64_t *nout)
    {
        FILE *fp = static_cast<FILE*>(handle);
        *nout = fwrite(buffer, 1, size, fp);
        return ferror(fp);
    }
    static int truncate(void *handle, int64_t size)
    {
        FILE *fp = static_cast<FILE*>(handle);
        fflush(fp);
        return _chsize(_fileno(fp), size) < 0;
    }
};

struct MP4InputStreamCallbacks: public MP4IOCallbacks
{
    MP4InputStreamCallbacks()
    {
        static MP4IOCallbacks t = {
            get_size, seek, read, write, truncate
        };
        std::memset(this, 0, sizeof t);
        std::memcpy(this, &t, sizeof t);
    }

    static int64_t get_size(void *handle)
    {
        return static_cast<IInputStream*>(handle)->size();
    }
    static int seek(void *handle, int64_t pos)
    {
        return static_cast<IInputStream*>(handle)->seek(pos, SEEK_SET) < 0;
    }
    static int read(void *handle, void *buffer, int64_t size, int64_t *nin)
    {
        *nin =  static_cast<IInputStream*>(handle)->read(buffer, size);
        return *nin < 0;
    }
    static int write(void *handle, const void *buffer, int64_t size, int64_t *nout)
    {
        return 1;
    }
    static int truncate(void *handle, int64_t size)
    {
        return 1;
    }
};

#endif
