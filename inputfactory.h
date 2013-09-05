#ifndef INPUTFACTORY_H
#define INPUTFACTORY_H

#include "iointer.h"
#include "flacmodule.h"
#include "wvpacksrc.h"
#include "taksrc.h"
#include "libsndfilesrc.h"
#include "soxdsp.h"
#include "soxrmodule.h"

namespace input {
    class InputFactory {
        AudioStreamBasicDescription m_raw_format;
        bool m_is_raw;
        bool m_ignore_length;
        std::map<std::wstring, std::shared_ptr<ISeekableSource> > m_sources;
    private:
        InputFactory() : m_is_raw(false), m_ignore_length(false) {}
    public:
        std::shared_ptr<ISeekableSource> open(const wchar_t *path);
        static InputFactory *getInstance()
        {
            static InputFactory *instance = new InputFactory();
            return instance;
        }
        void setRawFormat(const AudioStreamBasicDescription &asbd)
        {
            m_raw_format = asbd;
            m_is_raw = true;
        }
        void setIgnoreLength(bool cond)
        {
            m_ignore_length = cond;
        }
        FLACModule libflac;
        WavpackModule libwavpack;
        TakModule libtak;
        LibSndfileModule libsndfile;
        SoxModule libsoxrate;
        SOXRModule libsoxr;
    };

    inline InputFactory *factory() { return InputFactory::getInstance(); }
}
#endif
