#ifndef __FIREFLY_INTERNAL_SCENE_H__
#define __FIREFLY_INTERNAL_SCENE_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "firefly-scene-private.h"


#define MAX_ANIMATION_BACKLOG (32)

#define STOP_ADVANCE          (0xff01)
#define STOP_FREE             (0xff02)

struct Node;
struct Scene;


typedef enum NodeFlag {
    NodeFlagNone           = 0,

    // Node has a parent and cannot be added elsewhere
    NodeFlagHasParent      = (1 << 0),

    // Node should be removed from the scene graph on the next sequence
    NodeFlagRemove         = (1 << 3),

    // Node is in an animations block; changes should be queued as
    // an animation target
//    NodeFlagCapturing       = (1 << 8),
    NodeFlagHidden         = (1 << 4),

} NodeFlag;


NodeFlag ffx_sceneNode_hasFlags(FfxNode node, NodeFlag flags);
void ffx_sceneNode_setFlags(FfxNode node, NodeFlag flags);
void ffx_sceneNode_clearFlags(FfxNode node, NodeFlag flags);


typedef struct Action {
    struct Action *nextAction;
    FfxNodeActionFunc actionFunc;
    // Action State here
} Action;

typedef struct Animation {
    struct Animation *nextAnimation;
    void *dispatchArg;
    Action *actions;
    struct Node *node;
    int32_t startTime;
    uint32_t stop;
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

    // The current animation being populated with actions
    Animation *pendingAnimation;

    // Node State here
} Node;

typedef struct Stats {
    uint32_t seqCount;
    uint32_t renderCount, minRenderSize, maxRenderSize, totalRenderSize;
} Stats;

typedef struct RenderBlock {
    struct RenderBlock *nextBlock;
    size_t offset;
    uint8_t *data;
} RenderBlock;

typedef struct RenderList {
    RenderBlock *blockHead;
    RenderBlock *blockTail;
    Render *head;
    Render *tail;
} RenderList;


typedef struct Scene {
    // Memory allocation
    FfxSceneAllocFunc allocFunc;
    FfxSceneFreeFunc freeFunc;
    FfxSceneAnimationSetupFunc setupFunc;
    FfxSceneAnimationDispatchFunc dispatchFunc;
    void *initArg;

    // The root (group) node
    Node *root;

    Stats stats;

    // Gloabl tick
    // Guarded by animationLock ??
    int32_t tick;

    // The head and tail of the current render list (may be null)
    // Guarded by renderLock
    Render *renderHead;
    Render *renderTail;

    // The head and tail of the animation list (may be null)
    // Guarded by animationLock
    Animation *animationHead;
    Animation *animationTail;

    StaticQueue_t animQueueBuffer;
    uint8_t animQueueStorageBuffer[MAX_ANIMATION_BACKLOG * sizeof(Animation*)];
    QueueHandle_t animQueue;

    StaticSemaphore_t renderLockData;
    SemaphoreHandle_t renderLock;

} Scene;


void renderLock(Scene *scene);
void renderUnlock(Scene *scene);




#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __FIREFLY_INTERNAL_SCENE_H__ */
