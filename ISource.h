#ifndef _ISOURCE_H
#define _ISOURCE_H

#include <vector>
#include <map>
#include <memory>
#include "CoreAudio/CoreAudioTypes.h"
#include "misc.h"

struct ISource {
    virtual ~ISource() {}
    virtual uint64_t length() const = 0;
    virtual const AudioStreamBasicDescription &getSampleFormat() const = 0;
    virtual const std::vector<uint32_t> *getChannels() const = 0;
    virtual int64_t getPosition() = 0;
    virtual size_t readSamples(void *buffer, size_t nsamples) = 0;
};

struct ISeekableSource: public ISource {
    virtual bool isSeekable() = 0;
    virtual void seekTo(int64_t offset) = 0;
};

struct ITagParser {
    virtual ~ITagParser() {}
    virtual const std::map<std::string, std::string> &getTags() const = 0;
};

struct IChapterParser {
    virtual ~IChapterParser() {}
    virtual const std::vector<misc::chapter_t> &getChapters() const = 0;
};

size_t readSamplesFull(ISource *src, void *buffer, size_t nsamples);

size_t readSamplesAsFloat(ISource *src, std::vector<uint8_t> *pivot,
                          std::vector<float> *floatBuffer, size_t nsamples);

size_t readSamplesAsFloat(ISource *src, std::vector<uint8_t> *pivot,
                          float *floatBuffer, size_t nsamples);

size_t readSamplesAsFloat(ISource *src, std::vector<uint8_t> *pivot,
                          std::vector<double> *floatBuffer, size_t nsamples);

size_t readSamplesAsFloat(ISource *src, std::vector<uint8_t> *pivot,
                          double *floatBuffer, size_t nsamples);

#endif
