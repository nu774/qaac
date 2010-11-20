#ifndef _CHANMAP_H
#define _CHANMAP_H

#include <vector>
#include <stdint.h>

uint32_t
GetChannelLayoutTagFromChannelMap(const std::vector<uint32_t>& chanmap);

#endif
