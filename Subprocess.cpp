#include "Subprocess.h"
#include "platformutil.h"
#include "strutil.h"
#include "util.h"
#ifndef _WIN32
#include <unistd.h>
#include <sys/wait.h>
#include <cerrno>
#include <cstring>
#endif

#ifdef _WIN32
namespace {
    std::wstring quote_windows_arg(const std::wstring &arg)
    {
        if (!arg.empty() &&
            arg.find_first_of(L" \t\n\v\"") == std::wstring::npos)
            return arg;
        std::wstring result = L"\"";
        for (auto it = arg.begin(); ; ++it) {
            size_t backslashes = 0;
            while (it != arg.end() && *it == L'\\') {
                ++it;
                ++backslashes;
            }
            if (it == arg.end()) {
                result.append(backslashes * 2, L'\\');
                break;
            } else if (*it == L'"') {
                result.append(backslashes * 2 + 1, L'\\');
                result.push_back(*it);
            } else {
                result.append(backslashes, L'\\');
                result.push_back(*it);
            }
        }
        result.push_back(L'"');
        return result;
    }
}

ChildProcess::ChildProcess(const std::vector<std::string> &argv)
    : m_waited(false), m_exitCode(0)
{
    HANDLE hr, hw;
    SECURITY_ATTRIBUTES sa = { sizeof(sa), 0, TRUE };
    if (!CreatePipe(&hr, &hw, &sa, 0))
        platform::throw_error("CreatePipe", GetLastError());
    std::shared_ptr<void> readEnd(hr, CloseHandle);
    if (!SetHandleInformation(hw, HANDLE_FLAG_INHERIT, 0)) {
        CloseHandle(hw);
        platform::throw_error("SetHandleInformation", GetLastError());
    }

    std::wstring cmdline;
    for (size_t i = 0; i < argv.size(); ++i) {
        if (i) cmdline += L' ';
        cmdline += quote_windows_arg(strutil::us2w(argv[i]));
    }
    std::vector<wchar_t> buf(cmdline.begin(), cmdline.end());
    buf.push_back(0);

    STARTUPINFOW si;
    std::memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput = readEnd.get();
    si.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    si.hStdError = GetStdHandle(STD_ERROR_HANDLE);
    PROCESS_INFORMATION pi;
    std::memset(&pi, 0, sizeof(pi));
    BOOL ok = CreateProcessW(0, buf.data(), 0, 0, TRUE, 0, 0, 0, &si, &pi);
    if (!ok) {
        CloseHandle(hw);
        platform::throw_error(argv[0], GetLastError());
    }
    CloseHandle(pi.hThread);
    m_process.reset(pi.hProcess, CloseHandle);

    int fd = _open_osfhandle(reinterpret_cast<intptr_t>(hw),
                             _O_WRONLY | _O_BINARY);
    CHECKCRT(fd < 0);
    FILE *fp = _fdopen(fd, "wb");
    CHECKCRT(!fp);
    m_stdin.reset(fp, std::fclose);
}

int ChildProcess::wait()
{
    if (m_waited) return m_exitCode;
    m_stdin.reset();
    WaitForSingleObject(m_process.get(), INFINITE);
    DWORD code = 0;
    GetExitCodeProcess(m_process.get(), &code);
    m_exitCode = static_cast<int>(code);
    m_waited = true;
    return m_exitCode;
}

ChildProcess::~ChildProcess()
{
    try { wait(); } catch (...) {}
}
#else
ChildProcess::ChildProcess(const std::vector<std::string> &argv)
    : m_waited(false), m_exitCode(0), m_pid(-1)
{
    int fds[2];
    if (pipe(fds) != 0)
        util::throw_crt_error("pipe");
    pid_t pid = fork();
    if (pid < 0) {
        close(fds[0]);
        close(fds[1]);
        util::throw_crt_error("fork");
    }
    if (pid == 0) {
        dup2(fds[0], STDIN_FILENO);
        close(fds[0]);
        close(fds[1]);
        std::vector<char*> cargv(argv.size() + 1);
        for (size_t i = 0; i < argv.size(); ++i)
            cargv[i] = const_cast<char*>(argv[i].c_str());
        cargv[argv.size()] = 0;
        execvp(cargv[0], cargv.data());
        _exit(127);
    }
    close(fds[0]);
    FILE *fp = fdopen(fds[1], "wb");
    if (!fp) {
        close(fds[1]);
        util::throw_crt_error("fdopen");
    }
    m_stdin.reset(fp, std::fclose);
    m_pid = pid;
}

int ChildProcess::wait()
{
    if (m_waited) return m_exitCode;
    m_stdin.reset();
    int status = 0;
    while (waitpid(m_pid, &status, 0) < 0 && errno == EINTR)
        ;
    if (WIFEXITED(status))
        m_exitCode = WEXITSTATUS(status);
    else if (WIFSIGNALED(status))
        m_exitCode = 128 + WTERMSIG(status);
    else
        m_exitCode = -1;
    m_waited = true;
    return m_exitCode;
}

ChildProcess::~ChildProcess()
{
    try { wait(); } catch (...) {}
}
#endif
