/**
 * @file audio.h
 * @brief Native Audio Backend Contract
 */

#ifndef PORT_AUDIO_H
#define PORT_AUDIO_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize SDL audio device and ring buffer.
 * @param sampleRate Target audio sample rate (e.g., 32768 or 44100 Hz).
 * @param bufferFrames Ring buffer capacity in frames.
 */
bool NativeAudio_Init(uint32_t sampleRate, size_t bufferFrames);

/**
 * @brief Push synthesized 16-bit interleaved stereo PCM audio samples into the output queue.
 * @param samples Pointer to stereo samples [L0, R0, L1, R1, ...].
 * @param frameCount Number of stereo frames (pairs of samples).
 */
void NativeAudio_PushSamples(const int16_t* samples, size_t frameCount);

/**
 * @brief Shutdown audio device and free buffers.
 */
void NativeAudio_Shutdown(void);

#ifdef __cplusplus
}
#endif

#endif // PORT_AUDIO_H
