#ifndef SOUNDIOOUTDEVICE_H
#define SOUNDIOOUTDEVICE_H

#include <atomic>
#include <cstdint>
#include <deque>
#include <memory>
#include <string>
#include <vector>
#include <soundio/soundio.h>
#ifdef __APPLE__
#include <CoreServices/CoreServices.h>
#include <AudioToolbox/AudioToolbox.h>
#else
#include "CoreAudio/CoreAudioTypes.h"
#endif
#include "ConsoleInputParser.h"
#include "PlaybackNotifier.h"

class SoundIoOutDevice {
    std::shared_ptr<struct SoundIo> m_soundio;
    std::shared_ptr<struct SoundIoDevice> m_device;
    std::shared_ptr<struct SoundIoRingBuffer> m_ringBuffer;
    std::shared_ptr<struct SoundIoOutStream> m_outstream;
    AudioStreamBasicDescription m_asbd;
    uint32_t m_chanmask = 0;
    int m_bytesPerFrame = 0;
    std::vector<char> m_pendingWrite;
    PlaybackNotifier m_notifier;
    ConsoleInputParser m_parser;
    bool m_hasPendingKeyEvent = false;
    PlaybackKeyEvent m_pendingKeyEvent;
    int64_t m_queuedFramesAtLastKeyEvent = 0;
    std::atomic<bool> m_discardPending{false};

    struct ChunkInfo {
        std::string trackName;
        int64_t startPosition;
        int64_t frameCount;
    };
    std::deque<ChunkInfo> m_pendingChunks;
    std::atomic<int64_t> m_framesConsumed{0};
    int64_t m_framesConsumedBaseline = 0;
    std::string m_lastAnnouncedTrack;
    bool m_hasAnnouncedAny = false;

public:
    static SoundIoOutDevice &instance()
    {
        static SoundIoOutDevice self;
        return self;
    }
    void open(const AudioStreamBasicDescription &asbd, uint32_t chanmask);
    void pushSamples(const void *data, size_t length, size_t nsamples,
                     const std::string &trackName, int64_t startPosition);
    void close();
    void drain();

    bool takePendingKeyEvent(PlaybackKeyEvent *event)
    {
        if (!m_hasPendingKeyEvent) return false;
        *event = m_pendingKeyEvent;
        m_hasPendingKeyEvent = false;
        return true;
    }
    int64_t queuedFrames() const;
    int64_t queuedFramesAtLastKeyEvent() const
    {
        return m_queuedFramesAtLastKeyEvent;
    }
    bool getAudiblePosition(std::string *trackName, int64_t *position) const;
    bool checkTrackChange(std::string *trackName);

private:
    SoundIoOutDevice() = default;
    ~SoundIoOutDevice() { close(); }
    SoundIoOutDevice(const SoundIoOutDevice&) = delete;
    SoundIoOutDevice &operator=(const SoundIoOutDevice&) = delete;

    static void staticWriteCallback(struct SoundIoOutStream *outstream,
                                    int frame_count_min, int frame_count_max);
    static void staticUnderflowCallback(struct SoundIoOutStream *outstream);
    void writeCallback(struct SoundIoOutStream *outstream, int frame_count_max);
    void handleConsoleInput();
    bool currentChunk(std::string *trackName, int64_t *position) const;
    void resetBufferedAudio();
};

#endif
