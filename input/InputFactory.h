#ifndef INPUTFACTORY_H
#define INPUTFACTORY_H

#include "ISource.h"

class InputFactory {
    AudioStreamBasicDescription m_raw_format;
    bool m_is_raw;
    bool m_ignore_length;
    std::map<std::wstring, std::shared_ptr<ISeekableSource> > m_sources;
private:
    InputFactory() : m_is_raw(false), m_ignore_length(false) {}
    InputFactory(const InputFactory&);
    InputFactory& operator=(InputFactory&);
public:
    static InputFactory &instance()
    {
        static InputFactory self;
        return self;
    }
    std::shared_ptr<ISeekableSource> open(const wchar_t *path);
    void setRawFormat(const AudioStreamBasicDescription &asbd)
    {
        m_raw_format = asbd;
        m_is_raw = true;
    }
    void setIgnoreLength(bool cond)
    {
        m_ignore_length = cond;
    }
    void close()
    {
        m_sources.clear();
    }
};

#endif
