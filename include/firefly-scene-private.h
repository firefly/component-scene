#ifndef __FIREFLY_SCENE_PRIVATE_H__
#define __FIREFLY_SCENE_PRIVATE_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#include "firefly-scene.h"


typedef void (*_FfxNodeDestroyFunc)(FfxNode node);

typedef void (*_FfxNodeSequenceFunc)(FfxNode node, FfxPoint worldPos);

typedef void (*_FfxNodeRenderFunc)(void *render, uint16_t *frameBuffer,
  FfxPoint origin, FfxSize size);

typedef void (*_FfxNodeDumpFunc)(FfxNode node, int indent);

// Puts a string representation of %%node%% in %%output%% returning the length.
//typedef int (*_FfxNodeStringFunc)(FfxNode node, char* output, size_t length);


typedef struct _FfxNodeVTable {
    _FfxNodeDestroyFunc destroyFunc;

    _FfxNodeSequenceFunc sequenceFunc;
    _FfxNodeRenderFunc renderFunc;

    _FfxNodeDumpFunc dumpFunc;
//    _FfxNodeStringFunc stringFunc;
} _FfxNodeVTable;


//typedef void (*_FfxNodeAnimateFunc)(void *arg, fixed_ffxt t, bool didStop);

// Move to src
/*
typedef struct _FfxAnimation {
    _FfxNodeAnimateFunc animateFunc;
    struct _FfxAnimation *nextAnimation;

    // Copied to each new animation pushed on the queue.
    uint32_t delay;    // Maybe move duration and delay to startTime?
    uint32_t duration;
    FfxCurve curve

    uint32_t endTime;
} _FfxAnimation;
*/

// Node setters (or animatable properties) should call this if animating.
// Returned void* is %%extraData%% bytes (can be cast to a struct) and
// automatically freed once the animation completes.
//void* ffx_sceneNode_animate(FfxScene scene, FfxNode node,
//  _FfxNodeAnimateFunc animateFunc, FfxCurve curve, uint32_t delay,
//  uint32_t duration, size_t extraData);

FfxNode ffx_scene_createNode(FfxScene scene, const _FfxNodeVTable *vtable,
  size_t stateSize);
void* ffx_sceneNode_getState(FfxNode node);

void ffx_sceneNode_free(FfxNode node);

bool ffx_sceneNode_isPreparingAnimations(FfxNode node);

FfxNode ffx_sceneNode_getNextSibling(FfxNode node);

void* ffx_scene_createRender(FfxNode node, size_t stateSize);

/**
 *  These functions can be used to create custom Nodes for
 *  the scene graph.
 */

//////////////////////////////
// Custom nodes

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
