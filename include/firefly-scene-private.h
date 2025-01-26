#ifndef __FIREFLY_SCENE_PRIVATE_H__
#define __FIREFLY_SCENE_PRIVATE_H__

/**
 *  These methods are only necessary for developing custom Node objects.
 */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "firefly-scene.h"


//////////////////////////////
// Methods (via vtable)

typedef void (*FfxNodeDestroyFunc)(FfxNode node);

typedef void (*FfxNodeSequenceFunc)(FfxNode node, FfxPoint worldPos);

typedef void (*FfxNodeRenderFunc)(void *render, uint16_t *frameBuffer,
  FfxPoint origin, FfxSize size);

typedef void (*FfxNodeDumpFunc)(FfxNode node, int indent);


typedef struct FfxNodeVTable {
    // Called when deallocating the Node
    FfxNodeDestroyFunc destroyFunc;

    // Called during sequencing and rendering
    FfxNodeSequenceFunc sequenceFunc;
    FfxNodeRenderFunc renderFunc;

    // Dumps the node details to the terminal
    FfxNodeDumpFunc dumpFunc;
} FfxNodeVTable;


//////////////////////////////
// Memory

void* ffx_scene_memAlloc(FfxScene scene, size_t size);
void ffx_scene_memFree(FfxScene scene, void *ptr);

void* ffx_sceneNode_memAlloc(FfxNode node, size_t size);
void ffx_sceneNode_memFree(FfxNode node, void *ptr);


//////////////////////////////
// Life-cycle

/**
 *  Creates a new Node with the %%vtable%% in %%scene%%.
 *
 *  An additional %%stateSize%% bytes are allocated and can be
 *  accessed using getState to store Node state.
 */
FfxNode ffx_scene_createNode(FfxScene scene, const FfxNodeVTable *vtable,
  size_t stateSize);

/**
 *  Gets a pointer to the state allocated in the createNode call.
 */
void* ffx_sceneNode_getState(FfxNode node);

/**
 *  Frees a Node.
 */
void ffx_sceneNode_free(FfxNode node);

/**
 *  Access a Node's sibling. @TOOD: Move this to internal
 */
FfxNode ffx_sceneNode_getNextSibling(FfxNode node);


//////////////////////////////
// Sequencing

// Used during sequencing to request rendering with the returned state.
void* ffx_scene_createRender(FfxNode node, size_t stateSize);


//////////////////////////////
// Animations

/**
 *  Returns whether the node is currently capturing animations.

 *  Use this in setters to determine whether to set a value or
 *  instead queue an animtion transition of the property.
 */
bool ffx_sceneNode_isCapturing(FfxNode node);

typedef void (*FfxNodeActionFunc)(FfxNode node, fixed_ffxt t, void *state);

/**
 *  Creates an action, which updates the %%node%% throughout an animation
 *  through repeated calls (during each step) to %%actionFunc%%.
 *
 *  The returned void* is %%stateSize%% bytes available to store any
 *  necessary state, passed to %%actionFunc%%.
 */
void* ffx_sceneNode_createAction(FfxNode node, size_t stateSize,
  FfxNodeActionFunc actionFunc);


//////////////////////////////
// Debugging

/**
 *  Dumps the scene to the terminal.
 */
void ffx_scene_dumpScene(FfxScene scene);


//////////////////////////////
// Utilities

typedef struct FfxClip {
    int16_t x, y;
    int16_t vpX, vpY;
    int16_t width, height;
} FfxClip;

/**
 *  Compute the clipped region %%origin%% and %%size%% within the
 *  viewport bounds %%vpOrigin*%% and %%vpSize%%.
 *
 *  If the object is entirely outside the viewport, the result width
 *  is 0.
 */
FfxClip ffx_scene_clip(FfxPoint origin, FfxSize size, FfxPoint vpOrigin,
  FfxSize vpSize);



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __FIREFLY_SCENE_PRIVATE_H__ */
