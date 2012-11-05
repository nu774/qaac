#include "libplatform/impl.h"

namespace mp4v2 { namespace platform { namespace prog {

typedef struct
{
    int newmode;
} _startupinfo;

extern "C"
int __wgetmainargs(int *, wchar_t ***, wchar_t ***, int, _startupinfo *);

CommandLine::CommandLine(int argc, char **argv)
{
    wchar_t **wargv, **envp;
    _startupinfo si = { 0 };
    __wgetmainargs(&_argc, &wargv, &envp, 1, &si);

    _vargv.resize(_argc + 1);

    size_t buffer_size = 0;
    for (int i = 0; i < _argc; ++i) {
	buffer_size +=
	    WideCharToMultiByte(CP_UTF8, 0, wargv[i], -1, 0, 0, 0, 0);
    }
    _commandline.resize(buffer_size);
    char *bp = &_commandline[0], *endp = bp + _commandline.size();
    for (int i = 0; i < _argc; ++i) {
	int rc = WideCharToMultiByte(CP_UTF8, 0, wargv[i], -1, bp,
				     endp - bp, 0, 0);
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
