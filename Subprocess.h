#ifndef SUBPROCESS_H
#define SUBPROCESS_H

#include <cstdio>
#include <memory>
#include <string>
#include <vector>
#ifndef _WIN32
#include <sys/types.h>
#endif

class ChildProcess {
public:
    explicit ChildProcess(const std::vector<std::string> &argv);
    ~ChildProcess();

    std::shared_ptr<FILE> stdinFile() { return m_stdin; }

    int wait();

private:
    std::shared_ptr<FILE> m_stdin;
    bool m_waited;
    int m_exitCode;
#ifdef _WIN32
    std::shared_ptr<void> m_process; // HANDLE
#else
    pid_t m_pid;
#endif
};

#endif
