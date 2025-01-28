#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "scene.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <esp_debug_helpers.h>

FfxNode assertNode(FfxNode node) {
    if (node == NULL) {
        printf("Error: node cannot be NULL\n");
        esp_backtrace_print(32);

        printf("HAULT.\n");
        while(1) { vTaskDelay(10000); }
    }

    return node;
}

//////////////////////////
// Life-cycle

FfxNode ffx_scene_createNode(FfxScene scene, const FfxNodeVTable *vtable,
  size_t stateSize) {

    Node *node = ffx_scene_memAlloc(scene, sizeof(Node) + stateSize);

    node->vtable = vtable;
    node->scene = scene;

    return node;
}

void* ffx_sceneNode_getState(FfxNode _node) {
    Node *node = _node;
    return &node[1];
}

void ffx_sceneNode_free(FfxNode _node) {

    Node *node = _node;

    // <Critical Section>
    animationLock(node->scene);

    // Clear all animations on this node; no onComplete is called
    Animation *animation = node->scene->animationHead;
    while (animation) {
        if (animation->node == node) { animation->node = NULL; }
        animation = animation->nextAnimation;
    }

    animationUnlock(node->scene);
    // <//Critical Section>

    node->vtable->destroyFunc(node);

    ffx_sceneNode_memFree(node, node);
}

void ffx_sceneNode_remove(FfxNode _node, bool dealloc) {
    Node *node = assertNode(_node);

    // <Critical Section>
    animationLock(node->scene);

    // @TODO: maybe this doesn't need to be done?
    //        in updateAnimations detect a removed node

    // Clear all animations on this node; no onComplete is called
    Animation *animation = node->scene->animationHead;
    while (animation) {
        if (animation->node == node) { animation->node = NULL; }
        animation = animation->nextAnimation;
    }

    animationUnlock(node->scene);
    // </Critical Section>

    if (dealloc) {
        node->flags |= NodeFlagRemove | NodeFlagFree;
    } else {
        node->flags |= NodeFlagRemove;
    }
}


//////////////////////////
// Flags

NodeFlag ffx_sceneNode_hasFlags(FfxNode _node, NodeFlag flags) {
    Node *node = _node;
    return node->flags & flags;
}

void ffx_sceneNode_setFlags(FfxNode _node, NodeFlag flags) {
    Node *node = _node;
    node->flags |= flags;
}

void ffx_sceneNode_clearFlags(FfxNode _node, NodeFlag flags) {
    Node *node = _node;
    node->flags &= ~flags;
}


//////////////////////////
// VTable access

void ffx_sceneNode_sequence(FfxNode _node, FfxPoint worldPoint) {
    Node *node = _node;
    node->vtable->sequenceFunc(_node, worldPoint);
}

void ffx_sceneNode_dump(FfxNode _node, size_t indent) {
    Node *node = _node;
    node->vtable->dumpFunc(_node, indent);
}


//////////////////////////
// Properties

FfxNode ffx_sceneNode_getScene(FfxNode _node) {
    Node *node = _node;
    return node->scene;
}

FfxNode ffx_sceneNode_getNextSibling(FfxNode _node) {
    Node *node = _node;
    return node->nextSibling;
}

FfxPoint ffx_sceneNode_getPosition(FfxNode _node) {
    Node *node = _node;
    return node->position;
}

static void setPosition(FfxNode _node, FfxPoint position) {
    Node *node = _node;
    node->position = position;
}

void ffx_sceneNode_setPosition(FfxNode _node, FfxPoint pos) {
    Node *node = _node;
    ffx_sceneNode_createPointAction(node, node->position, pos, setPosition);
}

void ffx_sceneNode_offsetPosition(FfxNode _node, FfxPoint offset) {
    Node *node = _node;
    ffx_sceneNode_setPosition(_node, (FfxPoint){
        .x = node->position.x + offset.x,
        .y = node->position.y + offset.y,
    });
}

typedef struct AnimatePositionState {
    FfxPoint position;

    uint32_t delay;
    uint32_t duration;
    FfxCurveFunc curve;
    FfxNodeAnimationCompleteFunc onComplete;
    void* arg;
} AnimatePositionState;

static void animatePositionSetup(FfxNode node, FfxNodeAnimation *animation,
  void *arg) {

    AnimatePositionState *state = arg;
    animation->delay = state->delay;
    animation->duration = state->duration;
    animation->curve = state->curve;
    animation->onComplete = state->onComplete;
    animation->arg = state->arg;

    ffx_sceneNode_setPosition(node, state->position);
}

void ffx_sceneNode_animatePosition(FfxNode node, FfxPoint position,
  uint32_t delay, uint32_t duration, FfxCurveFunc curve,
  FfxNodeAnimationCompleteFunc onComplete, void* arg) {

    AnimatePositionState state = { 0 };
    state.position = position;
    state.delay = delay;
    state.duration = duration;
    state.curve = curve;
    state.onComplete = onComplete;
    state.arg = arg;

    ffx_sceneNode_animate(node, animatePositionSetup, &state);
}


//////////////////////////
// Property Animators

typedef struct ColorState {
    color_ffxt v0;
    color_ffxt v1;
    FfxNodeActionSetColorFunc setFunc;
} ColorState;

static void animateColor(FfxNode node, fixed_ffxt t, void *_state) {
    ColorState *state = _state;
    state->setFunc(node, ffx_color_lerpfx(state->v0, state->v1, t));
}

bool ffx_sceneNode_createColorAction(FfxNode node, color_ffxt v0,
  color_ffxt v1, FfxNodeActionSetColorFunc setFunc) {

    if (!ffx_sceneNode_isCapturing(node)) {
        setFunc(node, v1);
        return false;
    }

    ColorState *state = ffx_sceneNode_createAction(node, sizeof(ColorState),
      animateColor);

    state->v0 = v0;
    state->v1 = v1;
    state->setFunc = setFunc;

    return true;
}


typedef struct SizeState {
    FfxSize v0;
    FfxSize v1;
    FfxNodeActionSetSizeFunc setFunc;
} SizeState;

static void animateSize(FfxNode node, fixed_ffxt t, void *_state) {
    SizeState *state = _state;
    FfxSize v0 = state->v0;
    FfxSize v1 = state->v1;

    state->setFunc(node, (FfxSize){
        .width = v0.width + scalarfx(v1.width - v0.width, t),
        .height = v0.height + scalarfx(v1.height - v0.height, t)
    });
}

bool ffx_sceneNode_createSizeAction(FfxNode node, FfxSize v0, FfxSize v1,
  FfxNodeActionSetSizeFunc setFunc) {

    if (!ffx_sceneNode_isCapturing(node)) {
        setFunc(node, v1);
        return false;
    }

    SizeState *state = ffx_sceneNode_createAction(node, sizeof(SizeState),
      animateSize);

    state->v0 = v0;
    state->v1 = v1;
    state->setFunc = setFunc;

    return true;
}


typedef struct PointState {
    FfxPoint v0;
    FfxPoint v1;
    FfxNodeActionSetPointFunc setFunc;
} PointState;

static void animatePoint(FfxNode node, fixed_ffxt t, void *_state) {
    PointState *state = _state;
    FfxPoint v0 = state->v0;
    FfxPoint v1 = state->v1;

    state->setFunc(node, (FfxPoint){
        .x = v0.x + scalarfx(v1.x - v0.x, t),
        .y = v0.y + scalarfx(v1.y - v0.y, t)
    });
}

bool ffx_sceneNode_createPointAction(FfxNode node, FfxPoint v0, FfxPoint v1,
  FfxNodeActionSetPointFunc setFunc) {

    if (!ffx_sceneNode_isCapturing(node)) {
        setFunc(node, v1);
        return false;
    }

    PointState *state = ffx_sceneNode_createAction(node, sizeof(PointState),
      animatePoint);

    state->v0 = v0;
    state->v1 = v1;
    state->setFunc = setFunc;

    return true;
}


//////////////////////////
// Animation

bool ffx_sceneNode_isCapturing(FfxNode _node) {
    Node *node = _node;
    return node->pendingAnimation != NULL;
}

void* ffx_sceneNode_createAction(FfxNode _node, size_t stateSize,
  FfxNodeActionFunc actionFunc) {

    Node *node = _node;

    if (node->pendingAnimation == NULL) {
        printf("cannot add animations; not capturing\n");
        return NULL;
    }

    Action *action = ffx_sceneNode_memAlloc(node, sizeof(Action) + stateSize);

    action->actionFunc = actionFunc;

    // Prepend the action to the list of actions on the pending animation
    action->nextAction = node->pendingAnimation->actions;
    node->pendingAnimation->actions = action;

    return &action[1];
}

void ffx_sceneNode_animate(FfxNode _node,
  FfxNodeAnimationSetupFunc animationsFunc, void *arg) {

    Node *node = _node;

    if (node->pendingAnimation != NULL) {
        printf("already capturing animation\n");
        return;
    }

    Animation *animation = ffx_sceneNode_memAlloc(_node, sizeof(Animation));

    animation->node = node;
    animation->info.curve = FfxCurveLinear;

    node->pendingAnimation = animation;

    // Setup the animation
    animationsFunc(_node, &(animation->info), arg);

    // <Cretical Section>
    animationLock(node->scene);

    // Set the time
    {
        Scene *scene = node->scene;

        animation->startTime = scene->tick;

        // Add the new animation to the animation list
        if (scene->animationHead == NULL) {
            scene->animationHead = scene->animationTail = animation;
        } else {
            scene->animationTail->nextAnimation = animation;
            scene->animationTail = animation;
        }
    }

    animationUnlock(node->scene);
    // </Cretical Section>

    node->pendingAnimation = NULL;
}

void ffx_sceneNode_stopAnimations(FfxNode _node, bool completeAnimations) {
    Node *node = _node;

    FfxSceneActionStop stop = FfxSceneActionStopCurrent;
    if (completeAnimations) { stop = FfxSceneActionStopFinal; }

    // <Critical Section>
    animationLock(node->scene);

    // @TODO: maybe this can be done with flags?
    // Note: nope, in case removed and re-added? Maybe drop support?

    Animation *animation = node->scene->animationHead;
    while (animation) {
        if (animation->node == _node && !animation->stop) {
            animation->stop = stop;
        }
        animation = animation->nextAnimation;
    }

    animationUnlock(node->scene);
    // </Critical Section>
}
