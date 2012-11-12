#ifndef INTEGER_SOURCE_H
#define INTEGER_SOURCE_H

#include <assert.h>
#include <random>
#include "iointer.h"
#include "cautil.h"

class Quantizer: public FilterBase {
    AudioStreamBasicDescription m_asbd;
    std::mt19937 m_mt;
    std::vector<float> m_fbuffer;
    std::vector<double> m_dbuffer;
public:
    Quantizer(const std::shared_ptr<ISource> &source, uint32_t bitdepth)
	: FilterBase(source)
    {
	const AudioStreamBasicDescription &asbd = source->getSampleFormat();
	m_asbd = cautil::buildASBDForPCM2(asbd.mSampleRate,
					  asbd.mChannelsPerFrame,
					  bitdepth, 32,
					  kAudioFormatFlagIsSignedInteger);
    }
    const AudioStreamBasicDescription &getSampleFormat() const
    {
	return m_asbd;
    }
    size_t readSamples(void *buffer, size_t nsamples);
private:
    void ditherInt(int *data, size_t count, unsigned depth);
    template <typename T>
    void ditherFloat(T *src, int *dst, size_t count, unsigned depth);
};

#endif
