#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include <map>
#include <windows.h>
#include "shared_ptr.h"
#include "util.h"

struct IRegAction {
    ~IRegAction() {}
    virtual void onKey(const std::wstring &key) = 0;
    virtual void onValueName(const std::wstring &name) = 0;
    virtual void onType(DWORD type) = 0;
    virtual void onStringValue(const std::wstring &data) = 0;
    virtual void onDwordValue(DWORD data) = 0;
    virtual void onHexValue(const std::vector<BYTE> &data) = 0;
    virtual void onEvalValue(const std::wstring &data) = 0;
};

class RegParser {
    int m_lineno;
    std::wstring m_token;
    x::shared_ptr<FILE> m_fp;
    IRegAction *m_action;
public:
    RegParser(): m_lineno(1) {}

    void parse(const x::shared_ptr<FILE> &fp, IRegAction *action);
private:
    int get()
    {
	int c = std::getwc(m_fp.get());
	if (c == '\n') ++m_lineno;
	return c;
    }
    void put(int c)
    {
	m_token.push_back(c);
    }
    void error(const std::string &msg)
    {
	std::string s = format("RegParser: %s at line %d",
		msg.c_str(), m_lineno);
	throw std::runtime_error(s);
    }
    int expect(int c)
    {
	int cc;
	if ((cc = get()) != c)
	    error(format("%c is expected", c));
	return cc;
    }
    int skipws();
    int newline(int c)
    {
	if (c == '\r') c = get();
	if (c != '\n') error("New line expected");
	return '\n';
    }
    int hexDigits(int c, int width=0);
    std::vector<BYTE> getRawValue();
    int version();
    int key(int c);
    int value(int c);
    int valueName(int c);
    int valueData(int c);
    int stringValue(int c);
    int hexType(int c);
    int hexValue(int c);
    int dwordValue(int c);
    int evalValue(int c);

    void onKey()
    {
	m_action->onKey(m_token);
	m_token.clear();
    }
    void onValueName()
    {
	m_action->onValueName(m_token);
	m_token.clear();
    }
    void onHexType()
    {
	DWORD type;
	std::swscanf(m_token.c_str(), L"%x", &type);
	m_action->onType(type);
	m_token.clear();
    }
    void onStringValue()
    {
	m_action->onStringValue(m_token);
	m_token.clear();
    }
    void onDwordValue()
    {
	DWORD value;
	std::swscanf(m_token.c_str(), L"%x", &value);
	m_action->onDwordValue(value);
	m_token.clear();
    }
    void onHexValue()
    {
	m_action->onHexValue(getRawValue());
	m_token.clear();
    }
    void onEvalValue()
    {
	m_action->onEvalValue(m_token);
	m_token.clear();
    }
};

struct RegEntry {
    DWORD type;
    std::vector<BYTE> value;

    RegEntry(): type(REG_NONE) {}
    RegEntry(const std::wstring &s): type(REG_SZ)
    {
	size_t len = (s.size() + 1) * sizeof(wchar_t);
	value.resize(len);
	std::memcpy(&value[0], s.c_str(), len);
    }
    RegEntry(DWORD v): type(REG_DWORD)
    {
	value.resize(sizeof(DWORD));
	std::memcpy(&value[0], &v, sizeof(DWORD));
    }
    RegEntry(DWORD t, const std::vector<BYTE> &d)
	: type(t), value(d) {}
};

class RegAction: public IRegAction {
    typedef std::map<std::wstring, RegEntry> section_t;
    typedef std::map<std::wstring, section_t> hive_t;

    std::wstring m_key;
    std::wstring m_valueName;
    DWORD m_type;
    std::wstring m_selfdir;
    hive_t m_entries;
public:
    RegAction()
    {
	std::wstring selfpath = GetModuleFileNameX();
	const wchar_t *fpos = PathFindFileNameW(selfpath.c_str());
	m_selfdir = selfpath.substr(0, fpos - selfpath.c_str());
    }

    void onKey(const std::wstring &key)
    {
	m_key = key;
    }
    void onValueName(const std::wstring &name)
    {
	m_valueName = name;
    }
    void onType(DWORD type)
    {
	m_type = type;
    }
    void onStringValue(const std::wstring &data)
    {
	m_entries[m_key][m_valueName] = RegEntry(data);
    }
    void onHexValue(const std::vector<BYTE> &data)
    {
	m_entries[m_key][m_valueName] = RegEntry(m_type, data);
    }
    void onDwordValue(DWORD data)
    {
	m_entries[m_key][m_valueName] = RegEntry(data);
    }
    void onEvalValue(const std::wstring &data)
    {
	m_entries[m_key][m_valueName] =
	    RegEntry(process_template(data, *this));
    }
    std::wstring operator()(const std::wstring &name)
    {
	return name == L"qaacdir" ? m_selfdir : L"";
    }
    void realize();
    void show();
};
