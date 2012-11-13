#ifndef MIXER_H
#define MIXER_H

#include <complex>
#include <deque>
#include "iointer.h"
#include "soxdsp.h"

typedef std::complex<double> complex_t;

class MatrixMixer: public FilterBase {
    bool m_mt;
    uint32_t m_shiftMask;
    int64_t m_position;
    double m_filter_gain;
    std::vector<std::vector<complex_t> > m_matrix;
    std::shared_ptr<lsx_fir_t> m_filter;
    std::vector<double> m_coefs;
    std::deque<double> m_syncque;
    std::vector<uint8_t> m_ibuffer;
    std::vector<double> m_fbuffer;
    DecodeBuffer<double> m_buffer;
    AudioStreamBasicDescription m_asbd;
    SoxModule m_module;
public:
    MatrixMixer(const std::shared_ptr<ISource> &source, const SoxModule &module,
		const std::vector<std::vector<complex_t> > &spec, bool mt,
		bool normalize=true);
    const AudioStreamBasicDescription &getSampleFormat() const
    {
	return m_asbd;
    }
    const std::vector<uint32_t> *getChannels() const { return 0; }
    int64_t getPosition() { return m_position; }
    size_t readSamples(void *buffer, size_t nsamples);
private:
    void initFilter();
    size_t phaseShift(size_t nsamples);
};

#endif
