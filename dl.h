#ifndef DL_H
#define DL_H

#include <string>
#include <stdexcept>
#include "win32util.h"
#include "strutil.h"

class AutoCast {
    FARPROC m_pointer;
public:
    AutoCast(FARPROC p): m_pointer(p) {}
    template <typename U>
    operator U*() { return reinterpret_cast<U*>(m_pointer); }
};

class DL {
    std::shared_ptr<HINSTANCE__> m_module;
public:
    DL() {}
    DL(HMODULE handle, bool own=true)
    {
        struct noop { static void call(HMODULE x) {} };
        if (own)
            m_module.reset(handle, FreeLibrary);
        else
            m_module.reset(handle, noop::call);
    }
    bool load(const std::wstring &path)
    {
        HMODULE handle = LoadLibraryW(path.c_str());
        if (handle) m_module.reset(handle, FreeLibrary);
        return loaded();
    }
    bool loaded() const { return m_module.get() != 0; }
    void reset() { m_module.reset(); }
    AutoCast fetch(const char *name)
    {
        return AutoCast(GetProcAddress(m_module.get(), name));
    }
};

#endif
