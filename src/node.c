#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "firefly-scene-private.h"

#include "scene.h"


//////////////////////////
// Life-cycle

FfxNode ffx_scene_createNode(FfxScene _scene, const _FfxNodeVTable *vtable,
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

FfxPoint ffx_sceneNode_getPosition(FfxNode _node) {
    Node *node = _node;
    return node->position;
}

void ffx_sceneNode_setPosition(FfxNode _node, FfxPoint pos) {
    Node *node = _node;
    node->position = pos;
}

void ffx_sceneNode_offsetPosition(FfxNode _node, FfxPoint offset) {
    Node *node = _node;
    node->position.x += offset.x;
    node->position.y += offset.y;
}

FfxNode ffx_sceneNode_getNextSibling(FfxNode _node) {
    Node *node = _node;
    return node->nextSibling;
}
