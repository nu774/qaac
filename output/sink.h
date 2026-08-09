#ifndef _SINK_H
#define _SINK_H

#include "ISink.h"
#include "platformutil.h"

class ADTSSink: public ISink {
    typedef std::shared_ptr<FILE> file_ptr_t;
    file_ptr_t m_fp;
    uint32_t m_sample_rate_index;
    uint32_t m_channel_config;
    bool m_seekable;
    std::vector<uint8_t> m_pce_data;
public:
    ADTSSink(const std::string &path, const std::vector<uint8_t> &cookie,
             bool append=false);
    ADTSSink(const std::shared_ptr<FILE> &fp,
             const std::vector<uint8_t> &cookie);
    void writeSamples(const void *data, size_t length, size_t nsamples);
private:
    void init(const std::vector<uint8_t> &cookie);
    void write(const void *data, size_t size)
    {
        if (::write(fileno(m_fp.get()), data, size) < 0)
            util::throw_crt_error("write failed");
    }
};

#endif
