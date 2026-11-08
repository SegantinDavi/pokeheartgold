/**
 * @file snd_native.cpp
 * @brief NitroSDK Sound subsystem routing to SDL Audio backend.
 */

#include "port_snd.h"
#include "audio/audio_backend.h"
#include <algorithm>
#include <vector>

namespace {

s32 gMasterVolume = SND_MASTER_VOLUME_MAX;

} // anonymous namespace

extern "C" {

void SND_Init(void) {
    gMasterVolume = SND_MASTER_VOLUME_MAX;
    NativeAudio_Init(DEFAULT_AUDIO_SAMPLE_RATE, DEFAULT_AUDIO_BUFFER_FRAMES);
}

void SND_SetMasterVolume(s32 volume) {
    gMasterVolume = std::clamp(volume, 0, static_cast<s32>(SND_MASTER_VOLUME_MAX));
}

s32 SND_GetMasterVolume(void) {
    return gMasterVolume;
}

void SND_StopSeq(s32 player) {
    (void)player;
}

void SND_Shutdown(void) {
    NativeAudio_Shutdown();
}

void SND_ProcessAudio(const int16_t* pcmStereo, size_t frames) {
    if (!pcmStereo || frames == 0) return;

    if (gMasterVolume == SND_MASTER_VOLUME_MAX) {
        NativeAudio_PushSamples(pcmStereo, frames);
    } else if (gMasterVolume == 0) {
        std::vector<int16_t> silence(frames * 2, 0);
        NativeAudio_PushSamples(silence.data(), frames);
    } else {
        std::vector<int16_t> scaled(frames * 2);
        float factor = static_cast<float>(gMasterVolume) / static_cast<float>(SND_MASTER_VOLUME_MAX);
        for (size_t i = 0; i < frames * 2; ++i) {
            scaled[i] = static_cast<int16_t>(pcmStereo[i] * factor);
        }
        NativeAudio_PushSamples(scaled.data(), frames);
    }
}

} // extern "C"
