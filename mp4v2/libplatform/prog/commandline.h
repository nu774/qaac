#ifndef MP4V2_PLATFORM_PROG_COMMANDLINE_H
#define MP4V2_PLATFORM_PROG_COMMANDLINE_H

namespace mp4v2 { namespace platform { namespace prog {
    class MP4V2_EXPORT CommandLine {
	int _argc;
	char **_argv;
	std::vector<char> _commandline;
	std::vector<char*> _vargv;
    public:
	CommandLine(int argc, char **argv);
	void get(int *argc, char ***argv);
    };
}}} // namespace mp4v2::platform::prog

#endif // MP4V2_PLATFORM_PROG_COMMANDLINE_H

