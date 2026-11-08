/**
 * @file port_snd.h
 * @brief NitroSDK Sound (SND) subsystem emulation and native audio routing.
 */

#ifndef PORT_SND_H
#define PORT_SND_H

#include <stdint.h>
#include <stddef.h>
#include <nitro/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SND_MASTER_VOLUME_MAX 127

void SND_Init(void);
void SND_SetMasterVolume(s32 volume);
s32 SND_GetMasterVolume(void);
void SND_StopSeq(s32 player);
void SND_Shutdown(void);

/* Stream synthesized audio into native audio device */
void SND_ProcessAudio(const int16_t* pcmStereo, size_t frames);

#ifdef __cplusplus
}
#endif

#endif // PORT_SND_H
