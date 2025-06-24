#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "scene.h"

// DEBUG
uint32_t ticks();


//////////////////////////
// Memory

void* ffx_scene_memAlloc(FfxScene _scene, size_t size) {
    Scene *scene = _scene;

    void *ptr = scene->allocFunc(size, scene->initArg);
    if (ptr == NULL) {
        printf("FAIL: could not allocate %d bytes\n", size);
    }
    memset(ptr, 0, size);

    return ptr;
}

void ffx_scene_memFree(FfxScene _scene, void *ptr) {
    Scene *scene = _scene;
    scene->freeFunc(ptr, scene->initArg);
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
  FfxSceneFreeFunc freeFunc, FfxSceneAnimationSetupFunc setupFunc,
  FfxSceneAnimationDispatchFunc dispatchFunc, void *initArg) {

    Scene *scene = (void*)allocFunc(sizeof(Scene), initArg);
    if (scene == NULL) { return NULL; }
    memset(scene, 0, sizeof(Scene));

    scene->allocFunc = allocFunc;
    scene->freeFunc = freeFunc;
    scene->setupFunc = setupFunc;
    scene->dispatchFunc = dispatchFunc;
    scene->initArg = initArg;
    scene->tick = xTaskGetTickCount();
    scene->root = ffx_scene_createGroup(scene);

    scene->animQueue = xQueueCreateStatic(MAX_ANIMATION_BACKLOG,
      sizeof(Animation*), scene->animQueueStorageBuffer,
      &scene->animQueueBuffer);

    if (scene->root == NULL) {
        freeFunc((void*)scene, initArg);
        return NULL;
    }

    return scene;
}

void ffx_scene_free(FfxScene _scene) {
    Scene *scene = _scene;

    // @TODO: free all children!

    scene->freeFunc((void*)scene, scene->initArg);
}

bool ffx_scene_walk(FfxScene _scene, FfxNodeVisitFunc enterFunc,
  FfxNodeVisitFunc exitFunc, void *arg) {

    Scene *scene = _scene;

    if (enterFunc) {
        if (!enterFunc(scene->root, arg)) {
            return false;
        }
    }

    ffx_sceneNode_walk(scene->root, enterFunc, exitFunc, arg);

    if (exitFunc) {
        if (!exitFunc(scene->root, arg)) {
            return false;
        }
    }

    return true;
}

typedef struct Search {
    FfxNodeTag tag;
    FfxNode node;
} Search;

static bool checkTag(FfxNode node, void *arg) {
    Search *search = arg;
    if (ffx_scene_isAnchor(node) &&
      ffx_sceneAnchor_getTag(node) == search->tag) {
        search->node = node;
        return false;
    }
    return true;
}

FfxNode ffx_sceneNode_findAnchor(FfxNode node, FfxNodeTag tag) {
    Search search = { .tag = tag, .node = NULL };;

    if (!ffx_sceneNode_walk(node, checkTag, NULL, &search)) {
        return search.node;
    }

    return NULL;
}

FfxNode ffx_scene_findAnchor(FfxScene _scene, FfxNodeTag tag) {
    Scene *scene = _scene;
    return ffx_sceneNode_findAnchor(scene->root, tag);
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
    int32_t now = scene->tick;

    // Add any queued animation actions
    while (true) {

        Animation *anim = NULL;
        BaseType_t result = xQueueReceive(scene->animQueue, &anim, 0);
        if (result != pdPASS) { break; }

        // Queued Advance Animations
        if (anim->stop == STOP_ADVANCE) {
            Animation *animation = scene->animationHead;
            while (animation) {
                if (animation->node == anim->node && !animation->stop) {
                    animation->startTime -= anim->startTime;;
                }
                animation = animation->nextAnimation;
            }
            ffx_scene_memFree(scene, animation);
            continue;
        }

        // Queued Stop Animations
        if (anim->stop) {
            Animation *animation = scene->animationHead;
            while (animation) {
                if (animation->node == anim->node && !animation->stop) {
                    animation->stop = anim->stop;
                }
                animation = animation->nextAnimation;
            }
            ffx_scene_memFree(scene, animation);
            continue;
        }

        // Queued Animation

        anim->startTime = now;

        // Add the new animation to the animation list
        if (scene->animationHead == NULL) {
            scene->animationHead = scene->animationTail = anim;
        } else {
            scene->animationTail->nextAnimation = anim;
            scene->animationTail = anim;
        }
    }


    // The list of completed animations; removed from the list by not freed
    Animation *completeHead = NULL;
    Animation *completeTail = NULL;

    Animation *prevAnimation = NULL;

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

        if (animation->node == NULL || animation->node->flags & NodeFlagRemove) {
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

    // Clean up complete animations
    animation = completeHead;
    while (animation) {
        Animation *nextAnimation = animation->nextAnimation;

        FfxSceneActionStop stop = animation->stop;

        // Call any completion callback
        if (animation->info.onComplete) {
            if (scene->dispatchFunc) {
                scene->dispatchFunc(animation->dispatchArg,
                  animation->info.onComplete, animation->node, stop,
                  animation->info.arg, scene->initArg);
            } else {
                animation->info.onComplete(animation->node, stop,
                  animation->info.arg);
            }
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

    scene->stats.seqCount++;

    // Update all animations
    updateAnimations(scene);

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
}


//////////////////////////
// Rendering

//#define RENDER_SIZE         (512)

void* ffx_scene_createRender(FfxNode _node, size_t stateSize) {

    Node *node = _node;
    Scene *scene = node->scene;

    size_t size = sizeof(Render) + stateSize;

    scene->stats.renderCount++;
    scene->stats.totalRenderSize += size;
    if (size > scene->stats.maxRenderSize) { scene->stats.maxRenderSize = size; }
    if (size < scene->stats.minRenderSize) { scene->stats.minRenderSize = size; }

    Render *render = ffx_sceneNode_memAlloc(_node, size);

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
// Debugging

void ffx_scene_dump(FfxScene _scene) {
    Scene *scene = _scene;
    ffx_sceneNode_dump(scene->root, 0);
}

void ffx_scene_dumpStats(FfxScene _scene) {
    Scene *scene = _scene;

    printf("Scene Stats: seqCount=%ld\n", scene->stats.seqCount);

    printf("  Render Alloc: count=%ld min=%ld max=%ld avg=%ld avgPerFrame=%ld\n",
      scene->stats.renderCount,
      scene->stats.minRenderSize, scene->stats.maxRenderSize,
      scene->stats.totalRenderSize / scene->stats.renderCount,
      scene->stats.totalRenderSize / scene->stats.seqCount
      );

    scene->stats.seqCount = 0;;

    scene->stats.renderCount = 0;;
    scene->stats.minRenderSize = 0;;
    scene->stats.maxRenderSize = 0;;
    scene->stats.totalRenderSize = 0;;
}

