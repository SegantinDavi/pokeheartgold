/**
 * @file audio_backend.cpp
 * @brief Thread-safe circular audio ring buffer and SDL Audio playback device.
 */

#include "audio_backend.h"
#include <SDL.h>
#include <cstring>
#include <algorithm>
#include <memory>

namespace PortAudio {

AudioRingBuffer::AudioRingBuffer(size_t capacityFrames)
    : capacityFrames_(capacityFrames + 1) { // Allocate +1 for full/empty distinction
    buffer_ = new int16_t[capacityFrames_ * 2];
    memset(buffer_, 0, capacityFrames_ * 2 * sizeof(int16_t));
}

AudioRingBuffer::~AudioRingBuffer() {
    delete[] buffer_;
}

size_t AudioRingBuffer::AvailableReadFrames() const {
    size_t w = writeIndex_.load(std::memory_order_acquire);
    size_t r = readIndex_.load(std::memory_order_relaxed);
    if (w >= r) {
        return w - r;
    }
    return capacityFrames_ - (r - w);
}

size_t AudioRingBuffer::AvailableWriteFrames() const {
    size_t r = readIndex_.load(std::memory_order_acquire);
    size_t w = writeIndex_.load(std::memory_order_relaxed);
    size_t currentFilled = (w >= r) ? (w - r) : (capacityFrames_ - (r - w));
    return (capacityFrames_ - 1) - currentFilled;
}

size_t AudioRingBuffer::Write(const int16_t* samples, size_t frameCount) {
    if (!samples || frameCount == 0) return 0;

    size_t available = AvailableWriteFrames();
    size_t toWrite = std::min(frameCount, available);
    if (toWrite == 0) return 0;

    size_t w = writeIndex_.load(std::memory_order_relaxed);

    size_t firstPart = std::min(toWrite, capacityFrames_ - w);
    memcpy(&buffer_[w * 2], samples, firstPart * 2 * sizeof(int16_t));

    size_t secondPart = toWrite - firstPart;
    if (secondPart > 0) {
        memcpy(&buffer_[0], &samples[firstPart * 2], secondPart * 2 * sizeof(int16_t));
    }

    size_t newW = (w + toWrite) % capacityFrames_;
    writeIndex_.store(newW, std::memory_order_release);
    return toWrite;
}

size_t AudioRingBuffer::Read(int16_t* dst, size_t frameCount) {
    if (!dst || frameCount == 0) return 0;

    size_t available = AvailableReadFrames();
    size_t toRead = std::min(frameCount, available);
    size_t r = readIndex_.load(std::memory_order_relaxed);

    if (toRead > 0) {
        size_t firstPart = std::min(toRead, capacityFrames_ - r);
        memcpy(dst, &buffer_[r * 2], firstPart * 2 * sizeof(int16_t));

        size_t secondPart = toRead - firstPart;
        if (secondPart > 0) {
            memcpy(&dst[firstPart * 2], &buffer_[0], secondPart * 2 * sizeof(int16_t));
        }

        size_t newR = (r + toRead) % capacityFrames_;
        readIndex_.store(newR, std::memory_order_release);
    }

    // Fill remaining with silence (0) on underrun
    if (toRead < frameCount) {
        memset(&dst[toRead * 2], 0, (frameCount - toRead) * 2 * sizeof(int16_t));
    }

    return toRead;
}

void AudioRingBuffer::Clear() {
    writeIndex_.store(0, std::memory_order_relaxed);
    readIndex_.store(0, std::memory_order_release);
    memset(buffer_, 0, capacityFrames_ * 2 * sizeof(int16_t));
}

} // namespace PortAudio

namespace {

std::unique_ptr<PortAudio::AudioRingBuffer> gAudioBuffer;
SDL_AudioDeviceID gAudioDeviceId = 0;

void SdlAudioCallback(void* userdata, Uint8* stream, int len) {
    (void)userdata;
    if (!gAudioBuffer || len <= 0) {
        memset(stream, 0, static_cast<size_t>(len));
        return;
    }

    int16_t* out = reinterpret_cast<int16_t*>(stream);
    size_t framesRequested = static_cast<size_t>(len) / (2 * sizeof(int16_t));
    gAudioBuffer->Read(out, framesRequested);
}

} // anonymous namespace

extern "C" {

bool NativeAudio_Init(uint32_t sampleRate, size_t bufferFrames) {
    if (sampleRate == 0) sampleRate = DEFAULT_AUDIO_SAMPLE_RATE;
    if (bufferFrames == 0) bufferFrames = DEFAULT_AUDIO_BUFFER_FRAMES;

    gAudioBuffer = std::make_unique<PortAudio::AudioRingBuffer>(bufferFrames);

    SDL_AudioSpec desired{};
    desired.freq = static_cast<int>(sampleRate);
    desired.format = AUDIO_S16SYS;
    desired.channels = 2; // Stereo
    desired.samples = 1024;
    desired.callback = SdlAudioCallback;
    desired.userdata = nullptr;

    SDL_AudioSpec obtained{};
    gAudioDeviceId = SDL_OpenAudioDevice(nullptr, 0, &desired, &obtained, 0);
    if (gAudioDeviceId > 0) {
        SDL_PauseAudioDevice(gAudioDeviceId, 0); // Start playback
    }

    return true;
}

void NativeAudio_PushSamples(const int16_t* samples, size_t frameCount) {
    if (gAudioBuffer) {
        gAudioBuffer->Write(samples, frameCount);
    }
}

size_t NativeAudio_GetQueuedFrames(void) {
    return gAudioBuffer ? gAudioBuffer->AvailableReadFrames() : 0;
}

void NativeAudio_Shutdown(void) {
    if (gAudioDeviceId > 0) {
        SDL_CloseAudioDevice(gAudioDeviceId);
        gAudioDeviceId = 0;
    }
    gAudioBuffer.reset();
}

} // extern "C"
