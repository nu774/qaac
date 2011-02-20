#ifndef _CHANMAP_H
#define _CHANMAP_H

#include <vector>
#include <GNUCompatibility/stdint.h> // To avoid conflict with QT

uint32_t
GetChannelLayoutTagFromChannelMap(const std::vector<uint32_t>& chanmap);

#endif
