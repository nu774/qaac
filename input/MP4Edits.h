#ifndef MP4_EDITS_H
#define MP4_EDITS_H

#include <cstdint>
#include <vector>

class MP4Edits {
public:
    typedef std::pair<int64_t, int64_t> entry_t;

    void addEntry(int64_t offset, int64_t duration);
    void clear()
    {
        m_edits.clear();
    }
    size_t count() const
    {
        return m_edits.size();
    }
    int64_t totalDuration() const
    {
        return m_total_duration;
    }
    int64_t mediaOffset(unsigned edit_index) const
    {
        return m_edits[edit_index].first;
    }
    int64_t duration(unsigned edit_index) const
    {
        return m_edits[edit_index].second;
    }

    int64_t startPosition(unsigned edit_index) const;
    int64_t endPosition(unsigned edit_index) const;
    unsigned editForPosition(int64_t position, int64_t *offset_in_edit) const;
    int64_t mediaOffsetForPosition(int64_t position) const;
    void scaleShift(double ratio);
    void shiftMediaOffset(int val);
private:
    int64_t computeTotalDuration() const;

    std::vector<entry_t> m_edits;
    int64_t m_total_duration = 0;
};

#endif
