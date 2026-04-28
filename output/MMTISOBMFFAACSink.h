#ifndef MMTISOBMFFAACSINK_H
#define MMTISOBMFFAACSINK_H

#include <cstdint>
#include <string>
#include <vector>
#include "MMTISOBMFFSinkBase.h"

class MMTISOBMFFAACSink: public MMTISOBMFFSinkBase {
public:
    MMTISOBMFFAACSink(const std::string &path, const std::vector<uint8_t> &cookie,
                       uint32_t gapless_mode);
private:
    MMTISOBMFFAACSink(const MMTISOBMFFAACSink &);
    MMTISOBMFFAACSink& operator=(const MMTISOBMFFAACSink &);
};

#endif
