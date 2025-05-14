
#include <stddef.h>
#include <stdio.h>

#include "firefly-scene-private.h"
#include "scene.h"

typedef struct AnchorNode {
    FfxNode child;
    FfxNodeTag tag;
} AnchorNode;


static bool walkFunc(FfxNode node, FfxNodeVisitFunc enterFunc,
  FfxNodeVisitFunc exitFunc, void* arg);
static void destroyFunc(FfxNode node);
static void sequenceFunc(FfxNode node, FfxPoint worldPos);
static void renderFunc(void *_render, uint16_t *_frameBuffer,
  FfxPoint origin, FfxSize size);
static void dumpFunc(FfxNode node, int indent);

static const char name[] = "AnchorNode";
static const FfxNodeVTable vtable = {
    .walkFunc = walkFunc,
    .destroyFunc = destroyFunc,
    .sequenceFunc = sequenceFunc,
    .renderFunc = renderFunc,
    .dumpFunc = dumpFunc,
    .name = name,
};


//////////////////////////
// Methods

static bool walkFunc(FfxNode node, FfxNodeVisitFunc enterFunc,
  FfxNodeVisitFunc exitFunc, void* arg) {

    if (enterFunc && !enterFunc(node, arg)) { return false; }

    AnchorNode *anchor = ffx_sceneNode_getState(node, &vtable);
    if (!ffx_sceneNode_walk(anchor->child, enterFunc, exitFunc, arg)) {
        return false;
    }

    if (exitFunc && !exitFunc(node, arg)) { return false; }
    return true;
}

static void destroyFunc(FfxNode node) {
    AnchorNode *anchor = ffx_sceneNode_getState(node, &vtable);
    if (anchor->child) {
        ffx_sceneNode_free(anchor->child);
    }
}

static void sequenceFunc(FfxNode node, FfxPoint worldPos) {
    AnchorNode *anchor = ffx_sceneNode_getState(node, &vtable);
    if (anchor->child == NULL) { return; }

    FfxPoint pos = ffx_sceneNode_getPosition(node);
    worldPos.x += pos.x;
    worldPos.y += pos.y;

    ffx_sceneNode_sequence(anchor->child, worldPos);
}

static void renderFunc(void *_render, uint16_t *_frameBuffer,
  FfxPoint origin, FfxSize size) {
}

static void dumpFunc(FfxNode node, int indent) {
    AnchorNode *anchor = ffx_sceneNode_getState(node, &vtable);

    for (int i = 0; i < indent; i++) { printf("  "); }
    printf("<Anchor tag=%d data=%p>\n", anchor->tag, &anchor[1]);
    ffx_sceneNode_dump(anchor->child, indent + 1);
}


//////////////////////////
// Life-cycle

FfxNode ffx_scene_createAnchor(FfxScene scene, FfxNodeTag tag, size_t dataSize,
  FfxNode child) {

    FfxNode node = ffx_scene_createNode(scene, &vtable,
      sizeof(AnchorNode) + dataSize);

    if (ffx_sceneNode_hasFlags(child, NodeFlagHasParent)) {
        printf("child already has a parent; not added\n");
        child = NULL;
    } else {
        ffx_sceneNode_setFlags(child, NodeFlagHasParent);
    }

    AnchorNode *anchor = ffx_sceneNode_getState(node, &vtable);
    anchor->tag = tag;
    anchor->child = child;

    return node;
}

bool ffx_scene_isAnchor(FfxNode node) {
    return ffx_scene_isNode(node, &vtable);
}


//////////////////////////
// Properties

FfxNodeTag ffx_sceneAnchor_getTag(FfxNode node) {
    AnchorNode *anchor = ffx_sceneNode_getState(node, &vtable);
    if (anchor == NULL) { return 0; }
    return anchor->tag;
}

void ffx_sceneAnchor_setTag(FfxNode node, FfxNodeTag tag) {
    AnchorNode *anchor = ffx_sceneNode_getState(node, &vtable);
    if (anchor == NULL) { return; }
    anchor->tag = tag;;
}

FfxNode ffx_sceneAnchor_getChild(FfxNode node) {
    AnchorNode *anchor = ffx_sceneNode_getState(node, &vtable);
    if (anchor == NULL) { return NULL; }
    return anchor->child;
}

void* ffx_sceneAnchor_getData(FfxNode node) {
    AnchorNode *anchor = ffx_sceneNode_getState(node, &vtable);
    if (anchor == NULL) { return NULL; }
    return &anchor[1];
}
