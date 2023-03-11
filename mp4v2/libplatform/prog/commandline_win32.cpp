#include "libplatform/impl.h"
#include <shellapi.h>

namespace mp4v2 { namespace platform { namespace prog {

CommandLine::CommandLine(int argc, char **argv)
{
    wchar_t **wargv = CommandLineToArgvW(GetCommandLineW(), &_argc);

    _vargv.resize(_argc + 1);

    size_t buffer_size = 0;
    for (int i = 0; i < _argc; ++i) {
        buffer_size += WideCharToMultiByte(CP_UTF8, 0, wargv[i], -1, 0, 0, 0, 0);
    }
    _commandline.resize(buffer_size);
    char *bp = &_commandline[0], *endp = bp + _commandline.size();
    for (int i = 0; i < _argc; ++i) {
        int rc = WideCharToMultiByte(CP_UTF8, 0, wargv[i], -1, bp, endp - bp, 0, 0);
        _vargv[i] = bp;
        bp += rc;
    }
}
void CommandLine::get(int *argc, char ***argv)
{
    *argc = _argc;
    *argv = &_vargv[0];
}

}}} // namespace mp4v2::platform::prog
