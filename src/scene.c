#include <stddef.h>
#include <stdio.h>
#include <string.h>

// For tick
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


#include "scene.h"


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
// Sequencing and Rendering

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


void ffx_scene_render(FfxScene _scene, uint16_t *fragment, FfxPoint origin,
  FfxSize size) {
    printf("RENDER: pos=%dx%d size=%dx%d\n", origin.x, origin.y, size.width,
      size.height);
    Scene *scene = _scene;

    Render *render = scene->renderHead;
    while (render) {
        render->renderFunc(&render[1], fragment, origin, size);
        render = render->nextRender;
    }
}

//////////////////////////
// Debugging

void ffx_scene_dump(FfxScene _scene) {
    Scene *scene = _scene;
    ffx_sceneNode_dump(scene->root, 0);
}



