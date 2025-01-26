#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "scene.h"


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

    node->vtable->destroyFunc(node);

    // @TODO: clear out any animations (may need a special stop value)

    ffx_sceneNode_memFree(node, node);
}

void ffx_sceneNode_remove(FfxNode _node, bool dealloc) {
    Node *node = _node;

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

bool ffx_sceneNode_isCapturing(FfxNode node) {
    return ffx_sceneNode_hasFlags(node, NodeFlagCapturing);
}

void* ffx_sceneNode_createAction(FfxNode _node, size_t stateSize,
  FfxNodeActionFunc actionFunc) {

    if (!ffx_sceneNode_hasFlags(_node, NodeFlagCapturing)) {
        printf("cannot add animations; not capturing\n");
        return NULL;
    }

    Node *node = _node;
    Scene *scene = node->scene;

    Action *action = ffx_sceneNode_memAlloc(node, sizeof(Action) + stateSize);

    action->actionFunc = actionFunc;

    // Add the action to the head of the list of actions on the current animation
    action->nextAction = scene->animationTail->actions;
    scene->animationTail->actions = action;

    return &action[1];
}

void ffx_sceneNode_animate(FfxNode _node,
  FfxNodeAnimationSetupFunc animationsFunc, void *arg) {

    if (ffx_sceneNode_hasFlags(_node, NodeFlagCapturing)) {
        printf("already capturing animation\n");
        return;
    }

    Node *node = _node;
    Scene *scene = node->scene;

    Animation *animation = ffx_sceneNode_memAlloc(_node, sizeof(Animation));

    animation->node = node;
    animation->startTime = scene->tick;
    animation->info.curve = FfxCurveLinear;

    // Add the new animation to the animation list
    if (scene->animationHead == NULL) {
        scene->animationHead = scene->animationTail = animation;
    } else {
        scene->animationTail->nextAnimation = animation;
        scene->animationTail = animation;
    }

    ffx_sceneNode_setFlags(_node, NodeFlagCapturing);
    animationsFunc(_node, &animation->info, arg);
    ffx_sceneNode_clearFlags(_node, NodeFlagCapturing);
}

void ffx_sceneNode_stopAnimations(FfxNode _node, bool completeAnimations) {
    Node *node = _node;
    Scene *scene = node->scene;

    FfxSceneActionStop stop = FfxSceneActionStopCurrent;
    if (completeAnimations) { stop = FfxSceneActionStopFinal; }

    Animation *animation = scene->animationHead;
    while (animation) {
        if (animation->node == _node && !animation->stop) {
            animation->stop = stop;
        }
    }
}
