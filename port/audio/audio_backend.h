/**
 * @file audio_backend.h
 * @brief Thread-safe PCM audio circular ring buffer and SDL Audio backend.
 */

#ifndef PORT_AUDIO_BACKEND_H
#define PORT_AUDIO_BACKEND_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
#include <atomic>
#include <vector>

namespace PortAudio {

class AudioRingBuffer {
public:
    explicit AudioRingBuffer(size_t capacityFrames);
    ~AudioRingBuffer();

    size_t Write(const int16_t* samples, size_t frameCount);
    size_t Read(int16_t* dst, size_t frameCount);
    size_t AvailableReadFrames() const;
    size_t AvailableWriteFrames() const;
    void Clear();

private:
    int16_t* buffer_;
    size_t capacityFrames_;
    alignas(64) std::atomic<size_t> writeIndex_{0};
    alignas(64) std::atomic<size_t> readIndex_{0};
};

} // namespace PortAudio

extern "C" {
#endif

#define DEFAULT_AUDIO_SAMPLE_RATE  32768
#define DEFAULT_AUDIO_BUFFER_FRAMES 4096

bool NativeAudio_Init(uint32_t sampleRate, size_t bufferFrames);
void NativeAudio_PushSamples(const int16_t* samples, size_t frameCount);
size_t NativeAudio_GetQueuedFrames(void);
void NativeAudio_Shutdown(void);

#ifdef __cplusplus
}
#endif

#endif // PORT_AUDIO_BACKEND_H
