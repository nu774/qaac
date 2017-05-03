#include "WaveOutSink.h"
#include "util.h"
#include <mmreg.h>

inline void mm_try(MMRESULT expr, const char *msg)
{
    if (expr != MMSYSERR_NOERROR) {
        wchar_t text[1024];
        waveOutGetErrorTextW(expr, text, 1024);
	throw std::runtime_error(strutil::format("WaveOut: %s",
                                                 strutil::w2us(text).c_str()));
    }
} 
#define TRYMM(expr) (void)(mm_try(expr, #expr))

void WaveOutDevice::open(const AudioStreamBasicDescription &format,
                         uint32_t chanmask)
{
    if (std::memcmp(&format, &m_asbd, sizeof(format)) == 0
        && chanmask == m_chanmask)
        return;
    close();

    m_asbd = format;
    m_chanmask = chanmask;
    memset(m_packets, 0, sizeof m_packets);

    for (size_t i = 0; i < NUMBUFFERS; ++i) {
	m_packets[i].dwFlags = WHDR_DONE;
	m_events[i] = CreateEventW(0, 1, 1, 0); /* initially set. */
    }

    WAVEFORMATEXTENSIBLE wfex = {{ 0 }};
    WAVEFORMATEX &wfx = wfex.Format;
    wfx.cbSize = sizeof wfx;
    if (format.mChannelsPerFrame > 2 || format.mBitsPerChannel > 16 ||
        (format.mFormatFlags & kAudioFormatFlagIsFloat))
        wfx.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
    else
        wfx.wFormatTag = WAVE_FORMAT_PCM;
    wfx.nChannels = format.mChannelsPerFrame;
    wfx.nSamplesPerSec = format.mSampleRate;
    wfx.wBitsPerSample = ((format.mBitsPerChannel + 7) & ~7);
    wfx.nBlockAlign = wfx.nChannels * (wfx.wBitsPerSample >> 3);
    wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;
    wfx.cbSize = 0;
    if (wfx.wFormatTag == WAVE_FORMAT_EXTENSIBLE) {
        wfx.cbSize = 22;
        wfex.Samples.wValidBitsPerSample = wfx.wBitsPerSample;
        wfex.dwChannelMask = chanmask;
        if (format.mFormatFlags & kAudioFormatFlagIsFloat)
            wfex.SubFormat = KSDATAFORMAT_SUBTYPE_IEEE_FLOAT;
        else
            wfex.SubFormat = KSDATAFORMAT_SUBTYPE_PCM;
    }
    HWAVEOUT device;
    TRYMM(waveOutOpen(&device, WAVE_MAPPER,
                      reinterpret_cast<LPCWAVEFORMATEX>(&wfex),
                      reinterpret_cast<DWORD_PTR>(staticWaveOutProc),
                      reinterpret_cast<DWORD_PTR>(this), CALLBACK_FUNCTION));
    m_device = std::shared_ptr<HWAVEOUT__>(device, waveOutClose);
}

void WaveOutDevice::writeSamples(const void *data, size_t length,
                                 size_t nsamples)
{
    void *bp = const_cast<void *>(data);
    if (m_asbd.mBitsPerChannel <= 8) {
        util::convert_sign(static_cast<uint32_t *>(bp),
                           nsamples * m_asbd.mChannelsPerFrame);
    }
    unsigned obpc = m_asbd.mBytesPerFrame / m_asbd.mChannelsPerFrame;
    unsigned nbpc = ((m_asbd.mBitsPerChannel + 7) & ~7) >> 3;
    util::pack(bp, &length, obpc, nbpc);
    size_t pos = m_ibuffer.size();
    m_ibuffer.resize(pos + length);
    std::memcpy(&m_ibuffer[pos], bp, length);
    if (m_ibuffer.size() < m_asbd.mSampleRate / NUMBUFFERS)
        return;

    DWORD n = WaitForMultipleObjects(util::sizeof_array(m_events), m_events,
                                     0, INFINITE);
    n -= WAIT_OBJECT_0;
    ResetEvent(m_events[n]);
    WAVEHDR& wh = m_packets[n];
    TRYMM(waveOutUnprepareHeader(m_device.get(), &wh, sizeof wh));
    m_buffers[n] = m_ibuffer;
    m_ibuffer.clear();
    wh.lpData = &m_buffers[n][0];
    wh.dwBufferLength = m_buffers[n].size();
    TRYMM(waveOutPrepareHeader(m_device.get(), &wh, sizeof wh));
    TRYMM(waveOutWrite(m_device.get(), &wh, sizeof wh));
}

void WaveOutDevice::close()
{
    WaitForMultipleObjects(NUMBUFFERS, m_events, 1, INFINITE);
    for (size_t i = 0; i < NUMBUFFERS; ++i)
	if (m_events[i]) CloseHandle(m_events[i]);
    memset(m_events, 0, sizeof m_events);
    memset(&m_asbd, 0, sizeof m_asbd);
    m_chanmask = 0;
    m_device.reset();
}

void
WaveOutDevice::waveOutProc(UINT uMsg, DWORD_PTR dwParam1, DWORD_PTR)
{
    if (uMsg == WOM_DONE) {
	LPWAVEHDR lpwh = reinterpret_cast<LPWAVEHDR>(dwParam1);
	SetEvent(m_events[lpwh - m_packets]);
    }
}
