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

typedef bool (*FfxNodeWalkFunc)(FfxNode node, FfxNodeVisitFunc enterFunc,
  FfxNodeVisitFunc exitFunc, void* arg);

typedef void (*FfxNodeSequenceFunc)(FfxNode node, FfxPoint worldPos);

typedef void (*FfxNodeRenderFunc)(void *render, uint16_t *frameBuffer,
  FfxPoint origin, FfxSize size);

typedef void (*FfxNodeDumpFunc)(FfxNode node, int indent);

typedef void (*FfxNodeDestroyFunc)(FfxNode node);


typedef struct FfxNodeVTable {
    FfxNodeWalkFunc walkFunc;

    // Called during sequencing and rendering
    FfxNodeSequenceFunc sequenceFunc;
    FfxNodeRenderFunc renderFunc;

    // Dumps the node details to the terminal
    FfxNodeDumpFunc dumpFunc;

    // Called when deallocating the Node
    FfxNodeDestroyFunc destroyFunc;

    const char* name;
} FfxNodeVTable;


bool ffx_sceneNode_walk(FfxNode node, FfxNodeVisitFunc enterFunc,
  FfxNodeVisitFunc exitFunc, void* arg);
void ffx_sceneNode_sequence(FfxNode node, FfxPoint worldPoint);
void ffx_sceneNode_dump(FfxNode node, size_t indent);
const char* ffx_sceneNode_getName(FfxNode _node);


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
 *  Returns true if %%node%% has the given %%vtable%%.
 *
 *  This is a convenience method for implementing the ``scene_is*``
 *  functions.
 */
bool ffx_scene_isNode(FfxNode node, const FfxNodeVTable *vtable);

/**
 *  Gets a pointer to the state allocated in the createNode call.
 */
void* ffx_sceneNode_getState(FfxNode node, const FfxNodeVTable *vtable);

/**
 *  Frees a Node. This should only be used internally when actually
 *  freeing a node that was removed.
 *
 *  Generally, the [[ffx_sceneNode_remove]] should be used, which will
 *  schedule the node for freeing on the next sequence.
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


typedef void (*FfxNodeActionSetColorFunc)(FfxNode node, color_ffxt color);

bool ffx_sceneNode_createColorAction(FfxNode node, color_ffxt c0,
  color_ffxt c1, FfxNodeActionSetColorFunc setColorFunc);


typedef void (*FfxNodeActionSetPointFunc)(FfxNode node, FfxPoint point);

bool ffx_sceneNode_createPointAction(FfxNode node, FfxPoint p0, FfxPoint p1,
  FfxNodeActionSetPointFunc setPointFunc);


typedef void (*FfxNodeActionSetSizeFunc)(FfxNode node, FfxSize size);

bool ffx_sceneNode_createSizeAction(FfxNode node, FfxSize s0, FfxSize s1,
  FfxNodeActionSetSizeFunc setSizeFunc);


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
