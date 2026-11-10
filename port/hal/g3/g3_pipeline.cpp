/**
 * @file g3_pipeline.cpp
 * @brief Fixed-function 3D geometry pipeline, matrix stack, and FBO readback.
 */

#include "g3_pipeline.h"
#include <cstring>

extern "C" {

void G3Pipeline_Init(Nds3DPipelineState* state) {
    if (!state) return;
    std::memset(state, 0, sizeof(*state));
    state->mtxMode = GX_MTXMODE_POSITION_VECTOR;
}

void G3Pipeline_Destroy(Nds3DPipelineState* state) {
    if (!state) return;
    std::memset(state, 0, sizeof(*state));
}

} // extern "C"
