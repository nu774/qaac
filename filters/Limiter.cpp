#include "Limiter.h"

namespace {
    template <typename T> T clip(T x, T low, T high)
    {
        return std::max(low, std::min(high, x));
    }
}

void SoftClipper::process(const float *in, size_t nin, float *out, size_t *nout)
{
    for (int n = 0; n < m_nchannels; ++n) {
        std::vector<float> &x = m_buffer[n];
        x.reserve(x.size() + nin);
        for (size_t i = 0; i < nin; ++i)
            x.push_back(clip(in[i * m_nchannels + n],
                             -3.0f * m_thresh, 3.0f * m_thresh));

        ssize_t limit = x.size();
        if (limit > 0 && nin > 0) {
            float last = x[limit-1];
            for (; limit > 0 && x[limit-1] * last > 0; --limit)
                ;
        }
        ssize_t end = m_processed[n];
        while (end < limit) {
            ssize_t peak_pos = end;
            for (; peak_pos < limit; ++peak_pos)
                if (x[peak_pos] > m_thresh || x[peak_pos] < -m_thresh)
                    break;
            if (peak_pos == limit)
                break;
            ssize_t start = peak_pos;
            float peak = std::abs(x[peak_pos]);

            while (start > end && x[peak_pos] * x[start] >= 0)
                --start;
            ++start;
            for (end = peak_pos + 1; end < limit; ++end) {
                if (x[peak_pos] * x[end] < 0)
                    break;
                float y = std::abs(x[end]);
                if (y > peak) {
                    peak = y;
                    peak_pos = end;
                }
            }
            if (peak < m_thresh * 2.0) {
                float a = (peak - m_thresh) / (peak * peak);
                if (x[peak_pos] > 0) a = - a;
                for (ssize_t i = start; i < end; ++i)
                    x[i] = x[i] + a * x[i] * x[i];
            } else {
                float u = peak, v = m_thresh;
                float a = (u - 2 * v) / (u * u * u);
                float b = (3 * v - 2 * u) / (u * u);
                if (x[peak_pos] < 0)
                    b *= -1.0;
                for (ssize_t i = start; i < end; ++i)
                    x[i] = x[i] + b * x[i] * x[i] + a * x[i] * x[i] * x[i];
            }
        }
        m_processed[n] = limit;
    }
    size_t prod = std::min(*nout, *std::min_element(m_processed.begin(),
                                                    m_processed.end()));
    size_t k = 0;
    for (size_t i = 0; i < prod; ++i)
        for (int n = 0; n < m_nchannels; ++n)
            out[k++] = m_buffer[n][i];

    for (int n = 0; n < m_nchannels; ++n) {
        std::vector<float> &x = m_buffer[n];
        if (x.size() && prod) {
            std::rotate(x.begin(), x.begin() + prod, x.end());
            x.resize(x.size() - prod);
            m_processed[n] -= prod;
        }
    }
    *nout = prod;
}
