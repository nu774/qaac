#ifndef _SOXRMODULE_H
#define _SOXRMODULE_H

#include <soxr.h>
#include "dl.h"

class SOXRModule {
    DL m_dl;
private:
    SOXRModule() {
        if (!load("libsoxr64.dll"))
            load("libsoxr.dll");
    }
    SOXRModule(const SOXRModule&);
    SOXRModule& operator=(const SOXRModule&);
public:
    static SOXRModule &instance() {
        static SOXRModule self;
        return self;
    }
    bool load(const std::string &path);
    bool loaded() const { return m_dl.loaded(); }

    const char *(*version)();
    soxr_t (*create)(double, double, unsigned,
                     soxr_error_t *,
                     soxr_io_spec_t const *,
                     soxr_quality_spec_t const *,
                     soxr_runtime_spec_t const *);
    const char *(*engine)(soxr_t);
    soxr_error_t (*set_input_fn)(soxr_t, soxr_input_fn_t, void *, size_t);
    size_t (*output)(soxr_t, soxr_out_t, size_t);
    void (*delete_)(soxr_t);
    /* XXX
     * the following 3 functions have weak ABI due to returning struct,
     * that is incompatible between MinGW GCC < 4.7 and MSVC
     */
    soxr_quality_spec_t (*quality_spec)(unsigned long, unsigned long);
    soxr_runtime_spec_t (*runtime_spec)(unsigned);
    soxr_io_spec_t (*io_spec)(soxr_datatype_t, soxr_datatype_t);
};

#endif

