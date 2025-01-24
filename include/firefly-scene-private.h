#ifndef __FIREFLY_SCENE_PRIVATE_H__
#define __FIREFLY_SCENE_PRIVATE_H__

/**
 *  These methods are only necessary for developing custom Node objects.
 */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "firefly-scene.h"


typedef void (*_FfxNodeDestroyFunc)(FfxNode node);

typedef void (*_FfxNodeSequenceFunc)(FfxNode node, FfxPoint worldPos);

typedef void (*_FfxNodeRenderFunc)(void *render, uint16_t *frameBuffer,
  FfxPoint origin, FfxSize size);

typedef void (*_FfxNodeDumpFunc)(FfxNode node, int indent);


typedef struct _FfxNodeVTable {
    // Called when deallocating the Node
    _FfxNodeDestroyFunc destroyFunc;

    // Called during sequencing and rendering
    _FfxNodeSequenceFunc sequenceFunc;
    _FfxNodeRenderFunc renderFunc;

    _FfxNodeDumpFunc dumpFunc;
} _FfxNodeVTable;


void* ffx_scene_memAlloc(FfxScene scene, size_t size);
void ffx_scene_memFree(FfxScene scene, void *ptr);

void* ffx_sceneNode_memAlloc(FfxNode node, size_t size);
void ffx_sceneNode_memFree(FfxNode node, void *ptr);

//////////////////////////////
// Life-cycle

FfxNode ffx_scene_createNode(FfxScene scene, const _FfxNodeVTable *vtable,
  size_t stateSize);

void* ffx_sceneNode_getState(FfxNode node);

void ffx_sceneNode_free(FfxNode node);

FfxNode ffx_sceneNode_getNextSibling(FfxNode node);


//////////////////////////////
// Sequencing

// Used during sequencing to request rendering with the returned state.
void* ffx_scene_createRender(FfxNode node, size_t stateSize);


//////////////////////////////
// Animations

bool ffx_sceneNode_isCapturing(FfxNode node);


typedef void (*FfxNodeAnimationFunc)(FfxNode node, fixed_ffxt t,
  void *state);

void* ffx_sceneNode_createAnimationAction(FfxNode node, size_t stateSize,
  FfxNodeAnimationFunc animationFunc);


//////////////////////////////
// Debugging

void ffx_scene_dumpScene(FfxScene scene);


//////////////////////////////
// Utilities

typedef struct FfxClip {
    int16_t x, y;
    int16_t vpX, vpY;
    int16_t width, height;
} FfxClip;

/**
 *  Compute the clipped region of %%obj*%% within the viewport %%vp*%%.
 *
 *  If the object is entirely outside the viewport, the result width is 0.
 */
FfxClip ffx_scene_clip(FfxPoint origin, FfxSize size, FfxPoint vpOrigin,
  FfxSize vpSize);



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __FIREFLY_SCENE_PRIVATE_H__ */
