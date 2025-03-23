#ifndef IINPUT_STREAM_H
#define IINPUT_STREAM_H

#include <stdint.h>

struct IInputStream {
    virtual ~IInputStream() {}
    virtual bool seekable() = 0;
    virtual int read(void *buff, unsigned size) = 0;
    virtual int64_t seek(int64_t off, int whence) = 0;
    virtual int64_t tell() = 0;
    virtual int64_t size() = 0;
};

#endif
