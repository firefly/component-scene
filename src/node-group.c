/**
 *  Scene Node: Group
 *
 *  A Group Node does not have any visible objects itself, but
 *  allows for hierarchal scenes with multiple "attached" objects
 *  to be moved together.
 *
 *  x, y       - co-ordinates in local space
 *  a:void*    - a pointer to the first child
 *  b:void*    - a pointer to the last child
 */

// The group node is a bit special and requires lower-level access
// to the Scene and Nodes than normal nodes, so we access scene.h
// instead of firefly-scene-private.h

#include <stddef.h>
#include <stdio.h>


#include "scene.h"


// From scene.c
//void _freeSequence(FfxScene scene, FfxPoint worldPos, FfxNode node);

/*
static void _updateAnimations(_Scene *scene, _Node *node) {

    _Node *animate = node->animate.node;

    // No animations
    if (animate == NULL) { return; }

    int32_t now = scene->tick;

    _Node *lastAnimate = NULL;

    uint16_t stopType = FfxSceneActionStopNormal;

    while (animate) {
        int32_t duration = animate->a.i32;
        int32_t endTime = animate->b.i32;

        _Node *prop = animate->nextNode;
        _Node *nextAnimate = prop->nextNode;

        if (stopType == FfxSceneActionStopNormal) {
            stopType = animate->pos.y;

            // Animation advance
            if (stopType == 0xfe) {
                stopType = FfxSceneActionStopNormal;

                animate->b.i32 -= animate->pos.x;
                endTime = animate->b.i32;

                animate->pos.x = 0;
                animate->pos.y = FfxSceneActionStopNormal;
            }
        }
        if (stopType) { endTime = now; }

        if (stopType != FfxSceneActionStopCurrent) {

            // Compute the curve-adjusted t
            fixed_ffxt t = FM_1;
            if (endTime > now) {
                t = FM_1 - (tofx(endTime - now) / duration);
                if (t > FM_1) { t = FM_1; }
            }

            t = prop->func.curveFunc(t);

            // Perform the animation
            animate->func.animateFunc(node, t, prop->a, prop->b);
        }

        // Remove animation
        if (now >= endTime) {

            // Make the previous AnimateNode link to the next
            if (lastAnimate) {
                lastAnimate->nextNode->nextNode = nextAnimate;
            } else {
                node->animate.node = nextAnimate;
            }

            // Add the node back to the free nodes
            prop->nextNode = scene->nextFree;
            scene->nextFree = animate;

            if (prop->animate.onComplete) {
                prop->animate.onComplete(scene, node, stopType);
            }

        } else {
            lastAnimate = animate;
        }

        // Advance to the next animation
        animate = nextAnimate;
    }
}
*/


typedef struct GroupNode {
    FfxNode firstChild;
    FfxNode lastChild;
} GroupNode;

static void destoryFunc(FfxNode node) {
    GroupNode *state = ffx_sceneNode_getState(node);

    FfxNode *child = state->firstChild;
    while (child) {
        FfxNode *nextChild = ffx_sceneNode_getNextSibling(child);
        ffx_sceneNode_free(child);
        child = nextChild;
    }
}

static void sequenceFunc(FfxNode node, FfxPoint worldPos) {
    FfxPoint pos = ffx_sceneNode_getPosition(node);
    worldPos.x += pos.x;
    worldPos.y += pos.y;

    GroupNode *state = ffx_sceneNode_getState(node);

    // I have children; visit them each
    FfxNode *previousChild = NULL;
    FfxNode *child = state->firstChild;
    while (child) {
        FfxNode *nextChild = ffx_sceneNode_getNextSibling(child);

        if (ffx_sceneNode_hasFlags(child, NodeFlagRemove)) {
            // Remove child

            // The tail is the child; point the tail to the previous child (NULL if firstChild)
            if (state->lastChild == child) { state->lastChild = previousChild; }

            if (previousChild) {
                // Not the first child; remove from the middle
                ((_Node*)previousChild)->nextSibling = nextChild;
            } else {
               // First child
                state->firstChild = nextChild;
            }

            ((_Node*)child)->nextSibling = NULL;

            if (ffx_sceneNode_hasFlags(child, NodeFlagFree)) {
                ffx_sceneNode_free(child);
            } else {
                ffx_sceneNode_clearFlags(child, NodeFlagHasParent |
                  NodeFlagRemove | NodeFlagFree);
            }

        } else {
            // Update the animations
            //_updateAnimations(scene, child);

            // Sequence the child
            ffx_sceneNode_sequence(child, worldPos);
            previousChild = child;
        }

        child = nextChild;
    }
}

static void renderFunc(void *_render, uint16_t *frameBuffer,
  FfxPoint origin, FfxSize size) {
}

static void dumpFunc(FfxNode node, int indent) {

    FfxPoint pos = ffx_sceneNode_getPosition(node);

    for (int i = 0; i < indent; i++) { printf("  "); }
    printf("<Group pos=%dx%x>\n", pos.x, pos.y);

    GroupNode *state = ffx_sceneNode_getState(node);
    FfxNode *child = state->firstChild;
    while (child) {
        ffx_sceneNode_dump(child, indent + 1);
        child = ffx_sceneNode_getNextSibling(child);
    }
}

static const _FfxNodeVTable vtable = {
    .destroyFunc = destoryFunc,
    .sequenceFunc = sequenceFunc,
    .renderFunc = renderFunc,
    .dumpFunc = dumpFunc,
};

FfxNode ffx_scene_createGroup(FfxScene scene) {
    return ffx_scene_createNode(scene, &vtable, sizeof(GroupNode));
}

FfxNode ffx_sceneGroup_getFirstChild(FfxNode node) {
    GroupNode *state = ffx_sceneNode_getState(node);
    return state->firstChild;
}

void ffx_sceneGroup_appendChild(FfxNode node, FfxNode child) {
    if (ffx_sceneNode_hasFlags(child, NodeFlagHasParent)) {
        printf("child already has a parent; not added\n");
        return;
    }
    ffx_sceneNode_setFlags(child, NodeFlagHasParent);

    GroupNode *state = ffx_sceneNode_getState(node);
    if (state->firstChild == NULL) {
        state->firstChild = state->lastChild = child;
    } else {
        FfxNode _lastChild = state->lastChild;
        _Node *lastChild = _lastChild;
        lastChild->nextSibling = child;
        state->lastChild = child;
    }
}
