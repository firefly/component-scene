#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "firefly-scene-private.h"

#include "scene.h"


//////////////////////////
// Life-cycle

FfxNode ffx_scene_createNode(FfxScene _scene, const FfxNodeVTable *vtable,
  size_t stateSize) {
    Scene *scene = _scene;

    size_t size = sizeof(Node) + stateSize;

    Node *node = (void*)scene->allocFunc(size, scene->allocArg);
    memset(node, 0, size);

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

    node->vtable->destroyFunc(_node);

    Scene *scene = node->scene;
    scene->freeFunc((void*)node, scene->allocArg);
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

typedef struct PositionState {
    FfxPoint p0, p1;
} PositionState;

static void animatePosition(FfxNode _node, fixed_ffxt t, void *_state) {

    Node *node = _node;

    PositionState *state = _state;
    FfxPoint p0 = state->p0;
    FfxPoint p1 = state->p1;

    node->position.x = p0.x + scalarfx(p1.x - p0.x, t);
    node->position.y = p0.y + scalarfx(p1.y - p0.y, t);
}

FfxPoint ffx_sceneNode_getPosition(FfxNode _node) {
    Node *node = _node;
    return node->position;
}

void ffx_sceneNode_setPosition(FfxNode _node, FfxPoint pos) {
    if (ffx_sceneNode_isCapturing(_node)) {

        PositionState *state = ffx_sceneNode_createAction(_node,
          sizeof(PositionState), animatePosition);

        state->p0 = ffx_sceneNode_getPosition(_node);
        state->p1 = pos;

        return;
    }

    Node *node = _node;
    node->position = pos;
}

void ffx_sceneNode_offsetPosition(FfxNode _node, FfxPoint offset) {
    Node *node = _node;
    ffx_sceneNode_setPosition(_node, (FfxPoint){
        .x = node->position.x + offset.x,
        .y = node->position.y + offset.y,
    });
}

FfxNode ffx_sceneNode_getNextSibling(FfxNode _node) {
    Node *node = _node;
    return node->nextSibling;
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

    size_t size = sizeof(Action) + stateSize;

    Action *action = (void*)scene->allocFunc(size, scene->allocArg);
    memset(action, 0, size);

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

    Animation *animation = (void*)scene->allocFunc(sizeof(Animation),
      scene->allocArg);
    memset(animation, 0, sizeof(Animation));

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
