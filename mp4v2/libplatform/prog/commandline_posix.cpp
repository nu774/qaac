#include "libplatform/impl.h"

namespace mp4v2 { namespace platform { namespace prog {

CommandLine::CommandLine(int argc, char **argv)
    : _argc(argc), _argv(argv)
{
    // do nothing
}
void CommandLine::get(int *argc, char ***argv)
{
    *argc = _argc;
    *argv = _argv;
}

}}} // namespace mp4v2::platform::prog
