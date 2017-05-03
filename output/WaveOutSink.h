#ifndef _WAVEOUTSINK_H
#define _WAVEOUTSINK_H

#include <stdint.h>
#include <vector>
#include <memory>
#include <windows.h>
#include <mmsystem.h>
#include "CoreAudio/CoreAudioTypes.h"
#include "ISink.h"

class WaveOutDevice {
    enum { NUMBUFFERS = 2 };
    std::shared_ptr<HWAVEOUT__> m_device;
    HANDLE m_events[NUMBUFFERS];
    WAVEHDR m_packets[NUMBUFFERS];
    std::vector<char> m_buffers[NUMBUFFERS];
    std::vector<char> m_ibuffer;
    AudioStreamBasicDescription m_asbd;
    uint32_t m_chanmask;
public:
    static WaveOutDevice &instance()
    {
        static WaveOutDevice self;
        return self;
    }
    void open(const AudioStreamBasicDescription &asbd, uint32_t chanmask);
    void writeSamples(const void *data, size_t length, size_t nsamples);
    void close();
private:
    WaveOutDevice()
        : m_chanmask(0)
    {
        memset(m_events, 0, sizeof m_events);
        memset(&m_asbd, 0, sizeof m_asbd);
    }
    WaveOutDevice(const WaveOutDevice&);
    WaveOutDevice& operator=(const WaveOutDevice&);

    static void CALLBACK
        staticWaveOutProc(HWAVEOUT hwo, UINT uMsg, DWORD_PTR dwInstance,
                          DWORD_PTR dwParam1, DWORD_PTR dwParam2)
    {
	WaveOutDevice *self = reinterpret_cast<WaveOutDevice*>(dwInstance);
	self->waveOutProc(uMsg, dwParam1, dwParam2);
    }
    void waveOutProc(UINT uMsg, DWORD_PTR dwParam1, DWORD_PTR dwParam2);
};

class WaveOutSink: public ISink {
    WaveOutDevice& m_device;
public:
    WaveOutSink(const AudioStreamBasicDescription &format, uint32_t chanmask)
        : m_device(WaveOutDevice::instance())
    {
        m_device.open(format, chanmask);
    }
    void writeSamples(const void *data, size_t length, size_t nsamples)
    {
        m_device.writeSamples(data, length, nsamples);
    }
};

#endif
