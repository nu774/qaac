#include "SOXRModule.h"

#define CHECK(expr) do { if (!(expr)) throw std::runtime_error("!!!"); } \
    while (0)

bool SOXRModule::load(const std::string &path)
{
    if (!m_dl.load(path)) return false;
    try {
        CHECK(version = m_dl.fetch("soxr_version"));
        CHECK(create = m_dl.fetch("soxr_create"));
        CHECK(engine = m_dl.fetch("soxr_engine"));
        CHECK(set_input_fn = m_dl.fetch("soxr_set_input_fn"));
        CHECK(output = m_dl.fetch("soxr_output"));
        CHECK(delete_ = m_dl.fetch("soxr_delete"));
        CHECK(quality_spec = m_dl.fetch("soxr_quality_spec"));
        CHECK(runtime_spec = m_dl.fetch("soxr_runtime_spec"));
        CHECK(io_spec = m_dl.fetch("soxr_io_spec"));
        return true;
    } catch (...) {
        m_dl.reset();
        return false;
    }
}
