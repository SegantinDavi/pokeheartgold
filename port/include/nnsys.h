/**
 * @file nnsys.h
 * @brief NitroSystem forwarder/stubs for native desktop compilation.
 */

#ifndef PORT_NNSYS_H
#define PORT_NNSYS_H

#include "nitro.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declare common NitroSystem allocator and G2D/G3D types */
typedef struct NNSFndAllocator NNSFndAllocator;
typedef struct NNSFndAllocatorFunc NNSFndAllocatorFunc;

struct NNSFndAllocatorFunc {
    void* (*pfnAlloc)(NNSFndAllocator* pAllocator, u32 size);
    void (*pfnFree)(NNSFndAllocator* pAllocator, void* block);
};

struct NNSFndAllocator {
    const NNSFndAllocatorFunc* pFunc;
    void* pHeap;
    u32   heapParam1;
    u32   heapParam2;
};

#ifdef __cplusplus
}
#endif

#endif // PORT_NNSYS_H
