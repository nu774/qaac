#include "SoundIoOutDevice.h"
#include <algorithm>
#include <chrono>
#include <cstring>
#include <stdexcept>
#include <thread>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <unistd.h>
#endif
#include "strutil.h"
#include "logging.h"
#include "cautil.h"

namespace {

const int kEscTimeoutMs = 30;

enum SoundIoFormat mapFormat(const ca::AudioStreamBasicDescription &asbd,
                             unsigned containerBytes)
{
    bool isFloat = !!(asbd.mFormatFlags & kAudioFormatFlagIsFloat);
    bool isSigned = !!(asbd.mFormatFlags & kAudioFormatFlagIsSignedInteger);
    bool isBE = !!(asbd.mFormatFlags & kAudioFormatFlagIsBigEndian);

    if (isFloat) {
        if (containerBytes == 4) return isBE ? SoundIoFormatFloat32BE : SoundIoFormatFloat32LE;
        if (containerBytes == 8) return isBE ? SoundIoFormatFloat64BE : SoundIoFormatFloat64LE;
    } else {
        switch (containerBytes) {
        case 1: return isSigned ? SoundIoFormatS8 : SoundIoFormatU8;
        case 2: if (isSigned) return isBE ? SoundIoFormatS16BE : SoundIoFormatS16LE;
                return isBE ? SoundIoFormatU16BE : SoundIoFormatU16LE;
        case 4: if (isSigned) return isBE ? SoundIoFormatS32BE : SoundIoFormatS32LE;
                return isBE ? SoundIoFormatU32BE : SoundIoFormatU32LE;
        }
    }
    throw std::runtime_error(
        strutil::format("libsoundio playback: unsupported sample container "
                        "(%s, %u bytes per channel)",
                        isFloat ? "float" : "int", containerBytes));
}

} // namespace

void SoundIoOutDevice::open(const ca::AudioStreamBasicDescription &asbd,
                            uint32_t chanmask)
{
    if (m_outstream && !std::memcmp(&asbd, &m_asbd, sizeof asbd)
        && chanmask == m_chanmask)
        return;
    close();

    unsigned containerBytes = asbd.mChannelsPerFrame
        ? asbd.mBytesPerFrame / asbd.mChannelsPerFrame : 0;
    enum SoundIoFormat format = mapFormat(asbd, containerBytes);

    struct SoundIo *soundio = soundio_create();
    if (!soundio)
        throw std::runtime_error("soundio_create: out of memory");
    m_soundio.reset(soundio, soundio_destroy);
    int err = soundio_connect(m_soundio.get());
    if (err)
        throw std::runtime_error(
            strutil::format("libsoundio connect: %s", soundio_strerror(err)));
    soundio_flush_events(m_soundio.get());

    int idx = soundio_default_output_device_index(m_soundio.get());
    if (idx < 0)
        throw std::runtime_error("libsoundio: no output device found");
    struct SoundIoDevice *device = soundio_get_output_device(m_soundio.get(), idx);
    if (!device)
        throw std::runtime_error("soundio_get_output_device: out of memory");
    m_device.reset(device, soundio_device_unref);
    if (m_device->probe_error)
        throw std::runtime_error(
            strutil::format("libsoundio device probe: %s",
                            soundio_strerror(m_device->probe_error)));

    struct SoundIoOutStream *outstream = soundio_outstream_create(m_device.get());
    if (!outstream)
        throw std::runtime_error("soundio_outstream_create: out of memory");
    m_outstream.reset(outstream, soundio_outstream_destroy);
    m_outstream->format = format;
    m_outstream->sample_rate = static_cast<int>(asbd.mSampleRate);
    m_outstream->userdata = this;
    m_outstream->write_callback = staticWriteCallback;
    m_outstream->underflow_callback = staticUnderflowCallback;
    m_outstream->software_latency = 0.1;
    const struct SoundIoChannelLayout *defaultLayout =
        soundio_channel_layout_get_default(
            static_cast<int>(asbd.mChannelsPerFrame));
    if (!defaultLayout)
        throw std::runtime_error(
            strutil::format("libsoundio playback: no default channel "
                            "layout for %u channels",
                            asbd.mChannelsPerFrame));
    const struct SoundIoChannelLayout *bestLayout =
        soundio_best_matching_channel_layout(defaultLayout, 1,
                                             m_device->layouts,
                                             m_device->layout_count);
    m_outstream->layout = bestLayout ? *bestLayout : *defaultLayout;

    if ((err = soundio_outstream_open(m_outstream.get())))
        throw std::runtime_error(
            strutil::format("libsoundio outstream open: %s", soundio_strerror(err)));
    if (m_outstream->layout_error)
        LOG("WARNING: libsoundio channel layout: %s\n",
            soundio_strerror(m_outstream->layout_error));

    m_bytesPerFrame = m_outstream->bytes_per_frame;
    int capacity = m_bytesPerFrame * static_cast<int>(asbd.mSampleRate);
    struct SoundIoRingBuffer *ringBuffer =
        soundio_ring_buffer_create(m_soundio.get(), capacity);
    if (!ringBuffer)
        throw std::runtime_error("soundio_ring_buffer_create: out of memory");
    m_ringBuffer.reset(ringBuffer, soundio_ring_buffer_destroy);

    if ((err = soundio_outstream_start(m_outstream.get())))
        throw std::runtime_error(
            strutil::format("libsoundio outstream start: %s", soundio_strerror(err)));

    m_asbd = asbd;
    m_chanmask = chanmask;
}

void SoundIoOutDevice::pushSamples(const void *data, size_t length, size_t nsamples,
                                  const std::string &trackName, int64_t startPosition)
{
    int64_t consumed = m_framesConsumed.load(std::memory_order_relaxed)
        - m_framesConsumedBaseline;
    while (!m_pendingChunks.empty()
           && consumed >= m_pendingChunks.front().frameCount) {
        consumed -= m_pendingChunks.front().frameCount;
        m_framesConsumedBaseline += m_pendingChunks.front().frameCount;
        m_pendingChunks.pop_front();
    }
    m_pendingChunks.push_back({trackName, startPosition,
                               static_cast<int64_t>(nsamples)});

    const char *cdata = static_cast<const char*>(data);
    m_pendingWrite.insert(m_pendingWrite.end(), cdata, cdata + length);

    while (!m_pendingWrite.empty()) {
        int free = soundio_ring_buffer_free_count(m_ringBuffer.get());
        if (free > 0) {
            size_t n = std::min(static_cast<size_t>(free), m_pendingWrite.size());
            std::memcpy(soundio_ring_buffer_write_ptr(m_ringBuffer.get()),
                       m_pendingWrite.data(), n);
            soundio_ring_buffer_advance_write_ptr(m_ringBuffer.get(),
                                                  static_cast<int>(n));
            m_pendingWrite.erase(m_pendingWrite.begin(),
                                 m_pendingWrite.begin() + n);
            continue;
        }
        int timeout = m_parser.pending() ? kEscTimeoutMs : -1;
        PlaybackNotifier::WaitResult r = m_notifier.wait(timeout);
        if (r.timedOut && m_parser.pending()) {
            PlaybackKeyEvent ev = m_parser.timeout();
            if (ev.key != PlaybackKey::None) {
                resetBufferedAudio();
                m_hasPendingKeyEvent = true;
                m_pendingKeyEvent = ev;
                return;
            }
        }
        if (r.stdinReady) {
            handleConsoleInput();
            if (m_hasPendingKeyEvent)
                return;
        }
    }
}

int64_t SoundIoOutDevice::queuedFrames() const
{
    if (!m_bytesPerFrame)
        return 0;
    int fill = soundio_ring_buffer_fill_count(m_ringBuffer.get());
    int64_t frames = (fill + static_cast<int64_t>(m_pendingWrite.size()))
        / m_bytesPerFrame;
    frames += static_cast<int64_t>(m_outstream->software_latency * m_asbd.mSampleRate);
    return frames;
}

bool SoundIoOutDevice::currentChunk(std::string *trackName, int64_t *position) const
{
    int64_t consumed = m_framesConsumed.load(std::memory_order_relaxed)
        - m_framesConsumedBaseline;
    for (const ChunkInfo &chunk : m_pendingChunks) {
        if (consumed < chunk.frameCount) {
            *trackName = chunk.trackName;
            *position = chunk.startPosition + consumed;
            return true;
        }
        consumed -= chunk.frameCount;
    }
    return false;
}

bool SoundIoOutDevice::getAudiblePosition(std::string *trackName, int64_t *position) const
{
    return currentChunk(trackName, position);
}

bool SoundIoOutDevice::checkTrackChange(std::string *trackName)
{
    std::string track;
    int64_t position;
    if (!currentChunk(&track, &position))
        return false;
    if (m_hasAnnouncedAny && track == m_lastAnnouncedTrack)
        return false;
    m_lastAnnouncedTrack = track;
    m_hasAnnouncedAny = true;
    *trackName = track;
    return true;
}

void SoundIoOutDevice::close()
{
    m_outstream.reset();
    m_ringBuffer.reset();
    m_device.reset();
    m_soundio.reset();
    m_pendingWrite.clear();
    m_pendingChunks.clear();
    m_framesConsumed.store(0, std::memory_order_relaxed);
    m_framesConsumedBaseline = 0;
    m_lastAnnouncedTrack.clear();
    m_hasAnnouncedAny = false;
    std::memset(&m_asbd, 0, sizeof m_asbd);
    m_chanmask = 0;
    m_bytesPerFrame = 0;
    m_paused = false;
}

void SoundIoOutDevice::drain()
{
    if (!m_outstream || !m_ringBuffer)
        return;
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (std::chrono::steady_clock::now() < deadline) {
        int fill = soundio_ring_buffer_fill_count(m_ringBuffer.get());
        if (fill <= 0 && m_pendingWrite.empty())
            break;
        m_notifier.wait(50);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(
        static_cast<int64_t>((m_outstream->software_latency + 0.05) * 1000)));
}

void SoundIoOutDevice::staticWriteCallback(struct SoundIoOutStream *outstream,
                                          int /*frame_count_min*/,
                                          int frame_count_max)
{
    static_cast<SoundIoOutDevice*>(outstream->userdata)
        ->writeCallback(outstream, frame_count_max);
}

void SoundIoOutDevice::staticUnderflowCallback(struct SoundIoOutStream *)
{
}

void SoundIoOutDevice::writeCallback(struct SoundIoOutStream *outstream,
                                    int frame_count_max)
{
    struct SoundIoChannelArea *areas;
    int framesLeft = frame_count_max;
    int channels = outstream->layout.channel_count;
    int bps = outstream->bytes_per_sample;

    if (m_discardPending.exchange(false)) {
        int fill = soundio_ring_buffer_fill_count(m_ringBuffer.get());
        if (fill > 0)
            soundio_ring_buffer_advance_read_ptr(m_ringBuffer.get(), fill);
    }

    while (framesLeft > 0) {
        int frameCount = framesLeft;
        int err = soundio_outstream_begin_write(outstream, &areas, &frameCount);
        if (err)
            return;
        if (!frameCount)
            break;

        int available = soundio_ring_buffer_fill_count(m_ringBuffer.get());
        int bytesToCopy = std::min(frameCount * m_bytesPerFrame, available);
        int framesAvailable = bytesToCopy / m_bytesPerFrame;
        const char *src = soundio_ring_buffer_read_ptr(m_ringBuffer.get());

        for (int frame = 0; frame < framesAvailable; ++frame) {
            for (int ch = 0; ch < channels; ++ch) {
                std::memcpy(areas[ch].ptr, src, bps);
                areas[ch].ptr += areas[ch].step;
                src += bps;
            }
        }
        for (int frame = framesAvailable; frame < frameCount; ++frame) {
            for (int ch = 0; ch < channels; ++ch) {
                std::memset(areas[ch].ptr, 0, bps);
                areas[ch].ptr += areas[ch].step;
            }
        }

        soundio_ring_buffer_advance_read_ptr(m_ringBuffer.get(),
                                             framesAvailable * m_bytesPerFrame);
        m_framesConsumed.fetch_add(framesAvailable, std::memory_order_relaxed);

        err = soundio_outstream_end_write(outstream);
        if (err && err != SoundIoErrorUnderflow)
            return;
        framesLeft -= frameCount;
    }
    m_notifier.notify();
}

#ifdef _WIN32
void SoundIoOutDevice::handleConsoleInput()
{
    INPUT_RECORD ir = { 0 };
    DWORD nr = 0;
    if (!ReadConsoleInputW(GetStdHandle(STD_INPUT_HANDLE), &ir, 1, &nr) || nr == 0)
        return;
    if (ir.EventType != KEY_EVENT || !ir.Event.KeyEvent.bKeyDown)
        return;
    PlaybackKey key = PlaybackKey::None;
    switch (ir.Event.KeyEvent.wVirtualKeyCode) {
    case VK_LEFT:   key = PlaybackKey::Left; break;
    case VK_RIGHT:  key = PlaybackKey::Right; break;
    case VK_HOME:   key = PlaybackKey::Home; break;
    case VK_END:    key = PlaybackKey::End; break;
    case VK_PRIOR:  key = PlaybackKey::PageUp; break;
    case VK_NEXT:   key = PlaybackKey::PageDown; break;
    case VK_ESCAPE:
    case 'Q':       key = PlaybackKey::Quit; break;
    case VK_SPACE:
    case 'P':       key = PlaybackKey::Pause; break;
    default:        return; // not a key we care about
    }
    if (key != PlaybackKey::Pause)
        resetBufferedAudio();
    m_hasPendingKeyEvent = true;
    m_pendingKeyEvent = PlaybackKeyEvent(key);
}
#else
void SoundIoOutDevice::handleConsoleInput()
{
    unsigned char buf[64];
    ssize_t n = ::read(STDIN_FILENO, buf, sizeof buf);
    if (n <= 0)
        return;
    for (ssize_t i = 0; i < n; ++i) {
        PlaybackKeyEvent ev = m_parser.feed(buf[i]);
        if (ev.key != PlaybackKey::None) {
            if (!m_hasPendingKeyEvent && ev.key != PlaybackKey::Pause)
                resetBufferedAudio();
            m_hasPendingKeyEvent = true;
            m_pendingKeyEvent = ev;
        }
    }
}
#endif

void SoundIoOutDevice::resetBufferedAudio()
{
    m_queuedFramesAtLastKeyEvent = queuedFrames();
    m_pendingWrite.clear();
    m_pendingChunks.clear();
    m_framesConsumedBaseline = m_framesConsumed.load(std::memory_order_relaxed);
    if (m_ringBuffer)
        m_discardPending.store(true); // writeCallback() does the actual clear
    if (m_outstream) {
        soundio_outstream_clear_buffer(m_outstream.get());
    }
}
