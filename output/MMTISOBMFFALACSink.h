#ifndef MMTISOBMFFALACSINK_H
#define MMTISOBMFFALACSINK_H

#include <string>
#include <vector>
#include "MMTISOBMFFSinkBase.h"

class MMTISOBMFFALACSink: public MMTISOBMFFSinkBase {
public:
    MMTISOBMFFALACSink(const std::string &path, const std::vector<uint8_t> &cookie);
private:
    MMTISOBMFFALACSink(const MMTISOBMFFALACSink &);
    MMTISOBMFFALACSink& operator=(const MMTISOBMFFALACSink &);
};

#endif
