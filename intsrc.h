#ifndef INTEGER_SOURCE_H
#define INTEGER_SOURCE_H

#include <assert.h>
#include <random>
#include "iointer.h"
#include "cautil.h"

class IntegerSource: public FilterBase {
    AudioStreamBasicDescription m_asbd;
    std::mt19937 m_mt;
    std::uniform_real_distribution<double> m_dist;
    std::vector<uint8_t> m_ibuffer;
    std::vector<float> m_fbuffer;
public:
    IntegerSource(const std::shared_ptr<ISource> &source, uint32_t bitdepth)
	: FilterBase(source), m_dist(-0.5, 0.5)
    {
	const AudioStreamBasicDescription &asbd = source->getSampleFormat();
	m_asbd = cautil::buildASBDForPCM(asbd.mSampleRate,
				    asbd.mChannelsPerFrame,
				    bitdepth,
				    kAudioFormatFlagIsSignedInteger);
    }
    const AudioStreamBasicDescription &getSampleFormat() const
    {
	return m_asbd;
    }
    size_t readSamples(void *buffer, size_t nsamples);
private:
    double random() { return m_dist(m_mt); }
};

#endif
