#include "MP4Edits.h"
#include <algorithm>
#include <numeric>

void MP4Edits::addEntry(int64_t offset, int64_t duration)
{
    m_edits.emplace_back(offset, duration);
    m_total_duration = computeTotalDuration();
}

int64_t MP4Edits::computeTotalDuration() const
{
    return startPosition(m_edits.size());
}

int64_t MP4Edits::startPosition(unsigned edit_index) const
{
    return std::accumulate(m_edits.begin(), m_edits.begin() + edit_index, 0ULL,
                            [](uint64_t n, const entry_t &e) -> uint64_t {
                                return n + e.second;
                            });
}
int64_t MP4Edits::endPosition(unsigned edit_index) const
{
    return startPosition(edit_index + 1);
}

unsigned
MP4Edits::editForPosition(int64_t position, int64_t *offset_in_edit) const
{
    int64_t acc = 0;
    int64_t off = 0;
    size_t  i = 0;
    for (; i < m_edits.size(); ++i) {
        off =  position - acc;
        acc += m_edits[i].second;
        if (position < acc)
            break;
    }
    if (offset_in_edit) *offset_in_edit = off;
    return i == m_edits.size() ? i - 1 : i;
}

int64_t MP4Edits::mediaOffsetForPosition(int64_t position) const
{
    int64_t  off;
    unsigned edit = editForPosition(position, &off);
    return mediaOffset(edit) + off;
}

void MP4Edits::scaleShift(double ratio)
{
    std::for_each(m_edits.begin(), m_edits.end(), [&](entry_t & e) {
                    e.first = static_cast<int64_t>(e.first * ratio + .5);
                    e.second = static_cast<int64_t>(e.second * ratio + .5);
                    });
    m_total_duration = computeTotalDuration();
}

void MP4Edits::shiftMediaOffset(int val)
{
    std::for_each(m_edits.begin(), m_edits.end(), [val](entry_t & e) {
                    e.first = std::max(e.first + val, (int64_t)0);
                    });
}
