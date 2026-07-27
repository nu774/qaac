#include "PlaybackNotifier.h"
#include <stdexcept>

#ifdef _WIN32

PlaybackNotifier::PlaybackNotifier()
{
    m_event = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (!m_event)
        throw std::runtime_error("CreateEventW failed");
    m_stdin = GetStdHandle(STD_INPUT_HANDLE);
    DWORD mode;
    m_hasConsole = m_stdin && m_stdin != INVALID_HANDLE_VALUE
        && GetConsoleMode(m_stdin, &mode);
}

PlaybackNotifier::~PlaybackNotifier()
{
    if (m_event) CloseHandle(m_event);
}

void PlaybackNotifier::notify()
{
    SetEvent(m_event);
}

PlaybackNotifier::WaitResult PlaybackNotifier::wait(int timeoutMs)
{
    WaitResult result;
    HANDLE handles[2] = { m_event, m_stdin };
    DWORD count = m_hasConsole ? 2 : 1;
    DWORD timeout = timeoutMs < 0 ? INFINITE : static_cast<DWORD>(timeoutMs);
    DWORD n = WaitForMultipleObjects(count, handles, FALSE, timeout);
    if (n == WAIT_TIMEOUT) {
        result.timedOut = true;
    } else if (n == WAIT_OBJECT_0) {
        result.bufferReady = true;
        ResetEvent(m_event);
    } else if (n == WAIT_OBJECT_0 + 1) {
        result.stdinReady = true;
    }
    return result;
}

#else
#include <cerrno>
#include <cstring>
#include <string>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>

namespace {
void setNonBlocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}
}

PlaybackNotifier::PlaybackNotifier()
{
    int fds[2];
    if (pipe(fds) != 0)
        throw std::runtime_error(std::string("pipe: ") + std::strerror(errno));
    m_readFd = fds[0];
    m_writeFd = fds[1];
    setNonBlocking(m_readFd);
    setNonBlocking(m_writeFd);
}

PlaybackNotifier::~PlaybackNotifier()
{
    if (m_readFd >= 0) close(m_readFd);
    if (m_writeFd >= 0) close(m_writeFd);
}

void PlaybackNotifier::notify()
{
    unsigned char b = 0;
    for (;;) {
        ssize_t n = write(m_writeFd, &b, 1);
        if (n == 1) return;
        if (n < 0 && errno == EINTR) continue;
        return;
    }
}

PlaybackNotifier::WaitResult PlaybackNotifier::wait(int timeoutMs)
{
    struct pollfd pfds[2] = {
        { m_readFd, POLLIN, 0 },
        { STDIN_FILENO, POLLIN, 0 },
    };
    WaitResult result;
    int rc;
    do {
        rc = poll(pfds, 2, timeoutMs);
    } while (rc < 0 && errno == EINTR);

    if (rc == 0) {
        result.timedOut = true;
        return result;
    }
    if (rc < 0)
        return result;

    if (pfds[0].revents & POLLIN) {
        result.bufferReady = true;
        unsigned char buf[64];
        while (read(m_readFd, buf, sizeof buf) > 0) {}
    }
    if (pfds[1].revents & POLLIN)
        result.stdinReady = true;
    return result;
}

#endif // _WIN32
