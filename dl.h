#ifndef DL_H
#define DL_H

#include <string>
#include <stdexcept>
#ifdef _WIN32
#include "win32util.h"
#else
#include <dlfcn.h>
#endif
#include "strutil.h"

template <typename T>
class AutoCast {
    T *m_pointer;
public:
    AutoCast(T *p): m_pointer(p) {}
    template <typename U>
    operator U*() { return reinterpret_cast<U*>(m_pointer); }
};

#ifdef _WIN32
class DL {
    std::shared_ptr<HINSTANCE__> m_module;
public:
    DL() {}
    DL(HMODULE handle, bool takeOwn=true)
    {
	struct noop { static void call(HMODULE x) {} };
	if (takeOwn)
	    m_module.reset(handle, FreeLibrary);
	else
	    m_module.reset(handle, noop::call);
    }
    DL(const std::wstring &path)
    {
	HMODULE handle = LoadLibraryW(path.c_str());
	if (handle) m_module.reset(handle, FreeLibrary);
    }
    bool loaded() const { return m_module.get() != 0; }
    void reset() { m_module.reset(); }
    AutoCast<void> fetch(const char *name)
    {
	return AutoCast<void>(GetProcAddress(m_module.get(), name));
    }
};
#else
class DL {
    std::shared_ptr<void> m_module;
public:
    DL() {}
    DL(const std::string &path)
    {
	void *handle = dlopen(path.c_str(), RTLD_NOW);
	if (handle) m_module.reset(handle, dlclose);
    }
    DL(const std::wstring &path)
    {
	void *handle = dlopen(strutil::w2m(path).c_str(), RTLD_NOW);
	if (handle) m_module.reset(handle, dlclose);
    }
    bool loaded() const { return m_module.get() != 0; }
    void reset() { m_module.reset(); }
    AutoCast<void> fetch(const char *name)
    {
	return AutoCast<void>(dlsym(m_module.get(), name));
    }
};
#endif

#endif
