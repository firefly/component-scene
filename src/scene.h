#ifndef __FIREFLY_INTERNAL_SCENE_H__
#define __FIREFLY_INTERNAL_SCENE_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "firefly-scene-private.h"



struct _Node;
struct _Scene;


typedef enum NodeFlag {
    NodeFlagNone           = 0,

    // Node has a parent and cannot be added elsewhere
    NodeFlagHasParent      = (1 << 0),

    // Node should be removed from the scene graph on the next sequence
    NodeFlagRemove         = (1 << 3),

    // Node should be freed (after removal) on the next sequence
    NodeFlagFree           = (1 << 4),

    // Node is in an animations block; changes should be queued as
    // an animation target
    NodeFlagPapturing       = (1 << 8),
} NodeFlag;

NodeFlag ffx_sceneNode_hasFlags(FfxNode node, NodeFlag flags);
void ffx_sceneNode_setFlags(FfxNode node, NodeFlag flags);
void ffx_sceneNode_clearFlags(FfxNode node, NodeFlag flags);

void ffx_sceneNode_sequence(FfxNode node, FfxPoint worldPoint);
void ffx_sceneNode_dump(FfxNode node, size_t indent);

typedef struct _Node {
    const _FfxNodeVTable* vtable;
    struct _Scene *scene;
    FfxPoint position;
    uint32_t flags;
    //uint32_t state; set animateFunc to NULL to indicate start of block
//    _FfxAnimation *animationHead;
    FfxNode nextSibling;
} _Node;

struct Render;

typedef struct Render {
    struct Render *nextRender;
    _FfxNodeRenderFunc renderFunc;
} Render;

typedef struct _Scene {
    FfxSceneAllocFunc allocFunc;
    FfxSceneFreeFunc freeFunc;
    void *allocArg;

    // The root (group) node
    _Node *root;

    // Gloabl tick
    int32_t tick;

    // The head and tail of the current render list (may be null)
    Render *renderHead;
    Render *renderTail;

} _Scene;



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __FIREFLY_INTERNAL_SCENE_H__ */
