#ifndef CHAPTERS_H
#define CHAPTERS_H

#include <string>
#include <vector>
#include <stdint.h>

namespace chapters {
    /* chapter name, duration (in second) */
    typedef std::pair<std::wstring, double> entry_t;
    /* chapter name, absolute position (in second) */
    typedef std::pair<std::wstring, double> abs_entry_t;

    void load_from_file(const std::wstring &path,
			std::vector<abs_entry_t> *chapters,
			uint32_t codepage=0);
    void abs_to_duration(const std::vector<abs_entry_t> abs_ents,
			 std::vector<entry_t> *dur_ents,
			 double total_duration);
};

#endif
