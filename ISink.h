#ifndef _ISINK_H
#define _ISINK_H

#include <string>

struct ISink {
    virtual ~ISink() {}
    virtual void writeSamples(
            const void *data, size_t len, size_t nsamples) = 0;
};

struct ITagStore {
    virtual ~ITagStore() {}
    virtual void setTag(const std::string &key, const std::string &value) = 0;
};

#endif

