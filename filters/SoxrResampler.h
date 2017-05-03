#ifndef SOXRESAMPLER_H
#define SOXRESAMPLER_H

#include "SOXRModule.h"
#include "FilterBase.h"
#include "util.h"

class SoxrResampler: public FilterBase {
    int64_t m_position;
    uint64_t m_length;
    std::vector<uint8_t > m_pivot, m_buffer;
    std::shared_ptr<soxr> m_resampler;
    AudioStreamBasicDescription m_asbd;
    SOXRModule m_module;
public:
    SoxrResampler(const SOXRModule &module, const std::shared_ptr<ISource> &src,
                  unsigned rate);
    ~SoxrResampler() { m_resampler.reset(); }
    uint64_t length() const
    {
        return m_length;
    }
    const AudioStreamBasicDescription &getSampleFormat() const
    {
        return m_asbd;
    }
    size_t readSamples(void *buffer, size_t nsamples);
    int64_t getPosition() { return m_position; }
    const char *engine() { return m_module.engine(m_resampler.get()); }
private:
    static
    size_t staticInputProc(void *cookie, soxr_in_t *data, size_t nsamples)
    {
        SoxrResampler *self = static_cast<SoxrResampler*>(cookie);
        return self->inputProc(data, nsamples);
    }
    size_t inputProc(soxr_in_t *data, size_t nsamples);
};

#endif
