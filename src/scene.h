#ifndef __FIREFLY_INTERNAL_SCENE_H__
#define __FIREFLY_INTERNAL_SCENE_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "firefly-scene-private.h"



struct Node;
struct Scene;


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
    NodeFlagCapturing       = (1 << 8),

} NodeFlag;

NodeFlag ffx_sceneNode_hasFlags(FfxNode node, NodeFlag flags);
void ffx_sceneNode_setFlags(FfxNode node, NodeFlag flags);
void ffx_sceneNode_clearFlags(FfxNode node, NodeFlag flags);

void ffx_sceneNode_sequence(FfxNode node, FfxPoint worldPoint);
void ffx_sceneNode_dump(FfxNode node, size_t indent);


typedef struct Action {
    struct Action *nextAction;
    FfxNodeActionFunc actionFunc;
    // Action State here
} Action;


typedef struct Animation {
    struct Animation *nextAnimation;
    Action *actions;
    struct Node *node;
    uint32_t startTime;
    FfxSceneActionStop stop;
    FfxNodeAnimation info;
} Animation;


typedef struct Render {
    struct Render *nextRender;
    FfxNodeRenderFunc renderFunc;
    // Render State here
} Render;


typedef struct Node {
    const FfxNodeVTable* vtable;
    struct Scene *scene;
    FfxPoint position;
    uint32_t flags;
    FfxNode nextSibling;
    // Node State here
} Node;


typedef struct Scene {
    // Memory allocation
    FfxSceneAllocFunc allocFunc;
    FfxSceneFreeFunc freeFunc;
    void *allocArg;

    // The root (group) node
    Node *root;

    // Gloabl tick
    int32_t tick;

    // The head and tail of the current render list (may be null)
    Render *renderHead;
    Render *renderTail;

    // The head and tail of the animation list (may be null)
    Animation *animationHead;
    Animation *animationTail;

} Scene;



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __FIREFLY_INTERNAL_SCENE_H__ */
