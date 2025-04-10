#include <stddef.h>
#include <stdio.h>

#include "scene.h"


typedef struct GroupNode {
    FfxNode firstChild;
    FfxNode lastChild;
} GroupNode;


//////////////////////////
// Methods

static bool walkFunc(FfxNode node, FfxNodeVisitFunc enterFunc,
  FfxNodeVisitFunc exitFunc, void* arg) {

    GroupNode *state = ffx_sceneNode_getState(node);

    // I have children; visit them each
    FfxNode *child = state->firstChild;
    while (child) {
        FfxNode *nextChild = ffx_sceneNode_getNextSibling(child);

        if (enterFunc) {
            if (!enterFunc(child, arg)) { return false; }
        }

        ffx_sceneNode_walk(child, enterFunc, exitFunc, arg);

        if (exitFunc) {
            if (!exitFunc(child, arg)) { return false; }
        }

        child = nextChild;
    }

    return true;
}

static void destroyFunc(FfxNode node) {
    GroupNode *state = ffx_sceneNode_getState(node);

    FfxNode *child = state->firstChild;
    while (child) {
        FfxNode *nextChild = ffx_sceneNode_getNextSibling(child);
        ffx_sceneNode_free(child);
        child = nextChild;
    }
}

// @TODO: migrate this to walk
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
                ((Node*)previousChild)->nextSibling = nextChild;
            } else {
               // First child
                state->firstChild = nextChild;
            }

            ((Node*)child)->nextSibling = NULL;

            if (ffx_sceneNode_hasFlags(child, NodeFlagRemove)) {
                ffx_sceneNode_free(child);
            }

        } else {
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

static const FfxNodeVTable vtable = {
    .walkFunc = walkFunc,
    .destroyFunc = destroyFunc,
    .sequenceFunc = sequenceFunc,
    .renderFunc = renderFunc,
    .dumpFunc = dumpFunc,
};


//////////////////////////
// Life-cycle

FfxNode ffx_scene_createGroup(FfxScene scene) {
    return ffx_scene_createNode(scene, &vtable, sizeof(GroupNode));
}

bool ffx_scene_isGroup(FfxNode node) {
    return ffx_scene_isNode(node, &vtable);
}


//////////////////////////
// Properties

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
        Node *lastChild = _lastChild;
        lastChild->nextSibling = child;
        state->lastChild = child;
    }
}
