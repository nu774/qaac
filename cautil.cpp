#include "cautil.h"
#include "dl.h"

namespace cautil {
    std::string make_coreaudio_error(long code, const char *s)
    {
        std::stringstream ss;
        if (code == FOURCC('t','y','p','?'))
            return "Unsupported file type";
        else if (code == FOURCC('f','m','t','?'))
            return "Data format is not supported for this file type";
        int shift;
        for (shift = 0; shift < 32; shift += 8)
            if (!isprint((code >> shift) & 0xff))
                break;
        if (shift == 32)
            ss << s << ": "
               << static_cast<char>(code >> 24)
               << static_cast<char>((code >> 16) & 0xff)
               << static_cast<char>((code >> 8) & 0xff)
               << static_cast<char>(code & 0xff);
        else
            ss << s << ": " << code;
        return ss.str();
    }

#ifdef QAAC
    std::string CF2US(CFStringRef str)
    {
        CFIndex length = CFStringGetLength(str);
        if (!length) return "";
        std::vector<UniChar> buffer(length);
        CFRange range = { 0, length };
        CFStringGetCharacters(str, range, buffer.data());
        std::string result;
        utf8::utf16to8(std::begin(buffer), std::end(buffer), std::back_inserter(result));
        return result;
    }

    CFStringPtr US2CF(const std::string &s)
    {
        std::vector<UniChar> buffer;
        utf8::utf8to16(std::begin(s), std::end(s), std::back_inserter(buffer));
        CFStringRef sref = CFStringCreateWithCharacters(0, buffer.data(), buffer.size());
        return CFStringPtr(sref, CFRelease);
    }
#endif

}
