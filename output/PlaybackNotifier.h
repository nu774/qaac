#ifndef PLAYBACKNOTIFIER_H
#define PLAYBACKNOTIFIER_H

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

class PlaybackNotifier {
public:
    PlaybackNotifier();
    ~PlaybackNotifier();
    PlaybackNotifier(const PlaybackNotifier &) = delete;
    PlaybackNotifier &operator=(const PlaybackNotifier &) = delete;

    void notify();

    struct WaitResult {
        bool bufferReady = false;
        bool stdinReady = false;
        bool timedOut = false;
    };

    WaitResult wait(int timeoutMs);

private:
#ifdef _WIN32
    HANDLE m_event = nullptr;
    HANDLE m_stdin = nullptr;
    bool m_hasConsole = false;
#else
    int m_readFd = -1;
    int m_writeFd = -1;
#endif
};

#endif
