#ifndef _ISINK_H
#define _ISINK_H

#include <string>
#include "misc.h"

struct ISink {
    virtual ~ISink() {}
    virtual void writeSamples(
            const void *data, size_t len, size_t nsamples) = 0;
};

struct ITagStore {
    virtual ~ITagStore() {}
    virtual void setTag(const std::string &key, const std::string &value) = 0;
};

struct IChapterWriter {
    virtual ~IChapterWriter() {}
    virtual void setChapters(const std::vector<misc::chapter_t> &chapters) = 0;
};

#endif

