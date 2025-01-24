#include <stddef.h>
#include <stdio.h>
#include <string.h>

// For tick
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


#include "scene.h"


void* ffx_scene_memAlloc(FfxScene _scene, size_t size) {
    Scene *scene = _scene;

    void *ptr = scene->allocFunc(size, scene->allocArg);
    memset(ptr, 0, size);

    return ptr;
}

void ffx_scene_memFree(FfxScene _scene, void *ptr) {
    Scene *scene = _scene;
    scene->freeFunc(ptr, scene->allocArg);
}

void* ffx_sceneNode_memAlloc(FfxNode _node, size_t size) {
    Node *node = _node;
    return ffx_scene_memAlloc(node->scene, size);
}

void ffx_sceneNode_memFree(FfxNode _node, void *ptr) {
    Node *node = _node;
    ffx_scene_memFree(node->scene, ptr);
}



//////////////////////////
// Scene life-cycle

// Allocate a new Scene
FfxScene ffx_scene_init(FfxSceneAllocFunc allocFunc,
  FfxSceneFreeFunc freeFunc, void *allocArg) {

    Scene *scene = (void*)allocFunc(sizeof(Scene), allocArg);
    if (scene == NULL) { return NULL; }
    memset(scene, 0, sizeof(Scene));

    scene->allocFunc = allocFunc;
    scene->freeFunc = freeFunc;
    scene->allocArg = allocArg;
    scene->tick = xTaskGetTickCount();
    scene->root = ffx_scene_createGroup(scene);

    if (scene->root == NULL) {
        freeFunc((void*)scene, allocArg);
        return NULL;
    }

    return scene;
}

void ffx_scene_free(FfxScene _scene) {
    Scene *scene = _scene;

    // @TODO: free all children!

    scene->freeFunc((void*)scene, scene->allocArg);
}

FfxNode ffx_scene_root(FfxScene _scene) {
    Scene *scene = _scene;
    return scene->root;
}


//////////////////////////
// Sequencing

void ffx_scene_sequence(FfxScene _scene) {
    Scene *scene = _scene;

    // Delete the last render data
    if (scene->renderHead) {
        Render *render = scene->renderHead;
        scene->renderHead = scene->renderTail = NULL;

        while (render) {
            void *lastRender = render;
            render = render->nextRender;
            scene->freeFunc(lastRender, scene->allocArg);
        }
    }

    scene->tick = xTaskGetTickCount();

    // Sequence all the nodes
    ffx_sceneNode_sequence(scene->root, (FfxPoint){ .x = 0, .y = 0 });
}


//////////////////////////
// Rendering

void* ffx_scene_createRender(FfxNode _node, size_t stateSize) {

    Node *node = _node;
    Scene *scene = node->scene;

    size_t size = sizeof(Render) + stateSize;
    Render *render = (void*)scene->allocFunc(size, scene->allocArg);
    memset(render, 0, size);

    if (scene->renderHead == NULL) {
        scene->renderHead = scene->renderTail = render;
    } else {
        scene->renderTail->nextRender = render;
        scene->renderTail = render;
    }

    render->renderFunc = node->vtable->renderFunc;

    return &render[1];
}


void ffx_scene_render(FfxScene _scene, uint16_t *fragment, FfxPoint origin,
  FfxSize size) {

    Scene *scene = _scene;

    Render *render = scene->renderHead;
    while (render) {
        render->renderFunc(&render[1], fragment, origin, size);
        render = render->nextRender;
    }
}

//////////////////////////
// Animation



//////////////////////////
// Debugging

void ffx_scene_dump(FfxScene _scene) {
    Scene *scene = _scene;
    ffx_sceneNode_dump(scene->root, 0);
}



