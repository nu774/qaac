#ifndef MMTISOBMFFAACSINK_H
#define MMTISOBMFFAACSINK_H

#include <cstdint>
#include <deque>
#include <string>
#include <vector>
#include "MMTISOBMFFSinkBase.h"

class MMTISOBMFFAACSink: public MMTISOBMFFSinkBase, public IBitrateWriter {
public:
    MMTISOBMFFAACSink(const std::string &path, const std::vector<uint8_t> &cookie,
                       uint32_t gapless_mode);
    void writeSamples(const void *data, size_t length, size_t nsamples) override;
    void writeBitrates(int avgBitrate) override;
private:
    MMTISOBMFFAACSink(const MMTISOBMFFAACSink &);
    MMTISOBMFFAACSink& operator=(const MMTISOBMFFAACSink &);

    void updateMaxBitrate(bool finalize);

    struct SampleStat { uint32_t size; uint32_t duration; };
    std::deque<SampleStat> m_recentSamples;
    uint64_t m_recentSize = 0;
    uint64_t m_recentDuration = 0;
    uint32_t m_maxBitrate = 0;
};

#endif
