#ifndef DL_H
#define DL_H

#include <string>
#include <stdexcept>
#include <memory>
#ifdef _WIN32
#include "win32util.h"
#else
#include <dlfcn.h>
#endif
#include "strutil.h"

class AutoCast {
#ifdef _WIN32
    FARPROC m_pointer;
public:
    AutoCast(FARPROC p): m_pointer(p) {}
#else
    void *m_pointer;
public:
    AutoCast(void *p): m_pointer(p) {}
#endif
    template <typename U>
    operator U*() { return reinterpret_cast<U*>(m_pointer); }
};

class DL {
#ifdef _WIN32
    std::shared_ptr<HINSTANCE__> m_module;
public:
    DL() {}
    DL(HMODULE handle, bool own=true)
    {
        if (own)
            m_module.reset(handle, FreeLibrary);
        else
            m_module.reset(handle, [](HMODULE){});
    }
    bool load(const std::string &path)
    {
        HMODULE handle = LoadLibraryW(strutil::us2w(path).c_str());
        if (handle) m_module.reset(handle, FreeLibrary);
        return loaded();
    }
    bool loaded() const { return m_module.get() != 0; }
    void reset() { m_module.reset(); }
    AutoCast fetch(const char *name)
    {
        return AutoCast(GetProcAddress(m_module.get(), name));
    }
#else
    std::shared_ptr<void> m_module;
public:
    DL() {}
    DL(void *handle, bool own=true)
    {
        if (own)
            m_module.reset(handle, dlclose);
        else
            m_module.reset(handle, [](void*){});
    }
    bool load(const std::string &path)
    {
        void *handle = dlopen(path.c_str(), RTLD_NOW);
        if (handle) m_module.reset(handle, dlclose);
        return loaded();
    }
    bool loaded() const { return m_module.get() != 0; }
    void reset() { m_module.reset(); }
    AutoCast fetch(const char *name)
    {
        return AutoCast(dlsym(m_module.get(), name));
    }
#endif
};

#endif
