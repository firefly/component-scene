#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "scene.h"


//////////////////////////
// Mutex

void renderLock(Scene *scene) {
    xSemaphoreTake(scene->renderLock, portMAX_DELAY);
}

void renderUnlock(Scene *scene) {
    xSemaphoreGive(scene->renderLock);
}

void animationLock(Scene *scene) {
    xSemaphoreTake(scene->animLock, portMAX_DELAY);
}

void animationUnlock(Scene *scene) {
    xSemaphoreGive(scene->animLock);
}


//////////////////////////
// Memory

void* ffx_scene_memAlloc(FfxScene _scene, size_t size) {
    Scene *scene = _scene;

    void *ptr = scene->allocFunc(size, scene->allocArg);
    if (ptr == NULL) {
        printf("FAIL: could not allocate %d bytes\n", size);
    }
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
// Life-cycle

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

    scene->animLock = xSemaphoreCreateMutexStatic(&scene->animLockData);
    scene->renderLock = xSemaphoreCreateMutexStatic(&scene->renderLockData);

    return scene;
}

void ffx_scene_free(FfxScene _scene) {
    Scene *scene = _scene;

    // @TODO: free all children!

    scene->freeFunc((void*)scene, scene->allocArg);
}


//////////////////////////
// Properties

FfxNode ffx_scene_root(FfxScene _scene) {
    Scene *scene = _scene;
    return scene->root;
}


//////////////////////////
// Sequencing

static void updateAnimations(Scene *scene) {

    // The list of completed animations; removed from the list by not freed
    Animation *completeHead = NULL;
    Animation *completeTail = NULL;

    Animation *prevAnimation = NULL;

    // <Critical Section>
    animationLock(scene);

    int32_t now = scene->tick;

    Animation *animation = scene->animationHead;
    while (animation) {

        Animation *nextAnimation = animation->nextAnimation;

        FfxSceneActionStop stop = animation->stop;

        bool done = true;

        // Make sure we cast this to a signed value or the below "common"
        // type for the comparison will be unsigned. Since startTime can
        // be negative in the case of advanceAnimatin, we need to do all
        // maths in signed-land.
        int32_t delay = animation->info.delay;

        if (animation->node == NULL) {
            // Node was removed

        } else if (!stop && now <= animation->startTime + delay) {
            // Still running, but is in the delay
            done = false;

        } else if (animation->actions && stop != FfxSceneActionStopCurrent) {
            done = false;

            int32_t n = now - animation->info.delay;

            int32_t duration = animation->info.duration;
            int32_t endTime = animation->startTime + duration;

            fixed_ffxt t = FM_1;
            if (!stop && n < endTime) {
                t = FM_1 - (tofx(endTime - n) / duration);
                if (t > FM_1) { t = FM_1; }
            } else {
                done = true;
            }

            t = animation->info.curve(t);

            Action *action = animation->actions;
            while (action) {
                //printf("action: %p\n", action);
                action->actionFunc(animation->node, t, &action[1]);
                action = action->nextAction;
            }
        }

        if (done || stop) {
            if (prevAnimation == NULL) {
                scene->animationHead = nextAnimation;
            } else {
                prevAnimation->nextAnimation = nextAnimation;
            }

            animation->nextAnimation = NULL;

            if (completeHead == NULL) {
                completeHead = completeTail = animation;
            } else {
                completeTail->nextAnimation = animation;
                completeTail = animation;
            }

        } else {
            prevAnimation = animation;
        }

        animation = nextAnimation;
    }

    // No animations remain
    if (prevAnimation == NULL) { scene->animationHead = NULL; }
    scene->animationTail = prevAnimation;

    animationUnlock(scene);
    // </Critical Section>

    // Clean up complete animations
    animation = completeHead;
    while (animation) {
        Animation *nextAnimation = animation->nextAnimation;

        FfxSceneActionStop stop = animation->stop;

        // Call any completion callback
        if (animation->info.onComplete) {
            animation->info.onComplete(animation->node,
              animation->info.arg, stop);
        }

        // Remove the actions
        Action *action = animation->actions;
        while (action) {
            Action *nextAction = action->nextAction;
            ffx_scene_memFree(scene, action);
            action = nextAction;
        }

        // Free the animation
        ffx_scene_memFree(scene, animation);

        animation = nextAnimation;
    }
}

void ffx_scene_sequence(FfxScene _scene) {
    Scene *scene = _scene;

    // Update all animations
    updateAnimations(scene);

    // <Critical Section>
    renderLock(scene);

    // Delete the last render data
    if (scene->renderHead) {
        Render *render = scene->renderHead;
        scene->renderHead = scene->renderTail = NULL;

        while (render) {
            Render *nextRender = render->nextRender;
            ffx_scene_memFree(scene, render);
            render = nextRender;
        }
    }

    scene->tick = xTaskGetTickCount();

    // Sequence all the nodes
    ffx_sceneNode_sequence(scene->root, ffx_point(0, 0));

    renderUnlock(scene);
    // </Critical Section>
}


//////////////////////////
// Rendering

void* ffx_scene_createRender(FfxNode _node, size_t stateSize) {

    Node *node = _node;
    Scene *scene = node->scene;

    Render *render = ffx_sceneNode_memAlloc(_node, sizeof(Render) + stateSize);

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

    renderLock(scene);

    Render *render = scene->renderHead;
    while (render) {
        render->renderFunc(&render[1], fragment, origin, size);
        render = render->nextRender;
    }

    renderUnlock(scene);
}


//////////////////////////
// Debugging

void ffx_scene_dump(FfxScene _scene) {
    Scene *scene = _scene;
    ffx_sceneNode_dump(scene->root, 0);
}



