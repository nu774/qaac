#ifndef _ISINK_H
#define _ISINK_H

#include <string>
#include "catypes.h"
#include "misc.h"

struct ISink {
    virtual ~ISink() {}
    virtual void writeSamples(
            const void *data, size_t len, size_t nsamples) = 0;
};

struct IFinishWriteSink {
    virtual ~IFinishWriteSink() {}
    virtual void finishWrite(const ca::AudioFilePacketTableInfo &info) = 0;
};

struct ITagStore {
    virtual ~ITagStore() {}
    virtual void setTag(const std::string &key, const std::string &value) = 0;
};

struct IArtworkWriter {
    virtual ~IArtworkWriter() {}
    virtual void addArtwork(const std::vector<char> &data) = 0;
};

struct IChapterWriter {
    virtual ~IChapterWriter() {}
    virtual void setChapters(const std::vector<misc::chapter_t> &chapters) = 0;
};

struct IBitrateWriter {
    virtual ~IBitrateWriter() {}
    virtual void writeBitrates(int avgBitrate) = 0;
};

#endif

