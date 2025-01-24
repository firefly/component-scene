#ifndef __FIREFLY_SCENE_H__
#define __FIREFLY_SCENE_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdbool.h>
#include <stdint.h>

#include "firefly-color.h"
#include "firefly-curves.h"


// Scene Context Object (opaque; do not inspect or rely on internals)
typedef void* FfxScene;

// Scene Node Object (opaque; do not inspect or rely on internals)
typedef void* FfxNode;


// Point
typedef struct FfxPoint {
    int16_t x;
    int16_t y;
} FfxPoint;

//extern const FfxPoint PointZero;

// Size
typedef struct FfxSize {
    uint16_t width;
    uint16_t height;
} FfxSize;

//extern const FfxSize SizeZero;


// [ 1 bit: isBold ] [ 1 bit: reserved ] [ 6 bites: size ]
typedef enum FfxFont {
    FfxFontLarge     = 0x18,    // 24-point
    FfxFontMedium    = 0x14,    // 20-point
    FfxFontSmall     = 0x0f,    // 15-point
    FfxFontSizeMask  = 0x2f,

    FfxFontBold      = 0x80,    // Bold
} FfxFont;


// Animation

typedef enum FfxSceneActionStop {
    // Indicates the animation ran to completion as normal
    FfxSceneActionStopNormal      = 0,

    // Indicates the animation was stopped and left at its current value
    FfxSceneActionStopCurrent     = (1 << 1) | (0 << 0),

    // Indicates the animation was stopped and jumped to the end value
    FfxSceneActionStopFinal       = (1 << 1) | (1 << 0),
} FfxSceneActionStop;

typedef void (*FfxSceneAnimationCompletion)(FfxNode node,
  FfxSceneActionStop stopType, void *arg);


typedef uint8_t* (*FfxSceneAllocFunc)(size_t length, void *arg);
typedef void (*FfxSceneFreeFunc)(uint8_t* pointer, void* arg);


///////////////////////////////
// Scene

// Allocate a new Scene
FfxScene ffx_scene_init(FfxSceneAllocFunc allocFunc,
  FfxSceneFreeFunc freeFunc, void *allocArg);

// Release a Screen created using scene_init.
void ffx_scene_free(FfxScene scene);

// Debugging; dump the scene graph to the console
void ffx_scene_dump(FfxScene scene);

// Rendering
void ffx_scene_sequence(FfxScene scene);
void ffx_scene_render(FfxScene scene, uint16_t *fragment, FfxPoint origin,
  FfxSize size);

// Get the root node of the scene
FfxNode ffx_scene_root(FfxScene scene);


///////////////////////////////
// Node

/**
 * Schedule the %%node%% to be removed on the next sequence. If
 * %%dealloc%% the node will also have its destructor called and
 * be freed. Otherwise, it may be added back to the scene after
 * the next sequence. @TODO: allow this? Adding it before would
 * be catastoropic.
 */
void ffx_sceneNode_remove(FfxNode node, bool dealloc);

FfxNode ffx_sceneNode_getScene(FfxNode node);

FfxPoint ffx_sceneNode_getPosition(FfxNode node);
void ffx_sceneNode_setPosition(FfxNode node, FfxPoint pos);
void ffx_sceneNode_offsetPosition(FfxNode node, FfxPoint offset);


///////////////////////////////
// Group Node

FfxNode ffx_scene_createGroup(FfxScene scene);

FfxNode ffx_sceneGroup_getFirstChild(FfxNode node);
void ffx_sceneGroup_appendChild(FfxNode node, FfxNode child);


///////////////////////////////
// Node: Animations

//void ffx_sceneNode_beginAnimations(FfxNode node, uint32_t delay,
//  uint32_t duration, FfxCurve curve, FfxNodeAnimationsComplete onComplete,
//  void *arg);
//void ffx_sceneNode_commitAnimations(FfxNode node);

// IDEA:
//typedef void (*FfxNodeAnimateFunc)(FfxNode node, void *arg);
//void ffx_sceneNode_animate(FfxNode node, uint32_t delay, uint32_t duration,
//  FfxCurve curve, FfxNodeAnimateFunc animateFunc,
//  FfxNodeAnimationsCompleteFunc onComplete);

// IDEA2:
// Allocate immediately; use AnimationFunc for item to snapshot
// anything necessary and that can be updated in the setters;
// then the setters update that object.

typedef void (*FfxNodeAnimationCompleteFunc)(FfxNode node, void* arg,
  FfxSceneActionStop stopReason);

typedef struct FfxAnimation {
    uint32_t delay;         // Default: 0
    uint32_t duration;      // Default: 0
    FfxCurveFunc curve ;        // Default: Linear
    FfxNodeAnimationCompleteFunc onComplete; // Default: NULL
    void* arg;              // Default: NULL
} FfxAnimation;

typedef void (*FfxNodeAnimateFunc)(FfxNode node, FfxAnimation *animate,
  void *arg);

void ffx_sceneNode_animate(FfxNode node, FfxNodeAnimateFunc animationsFunc,
  void *arg);



// Returns true if node is running any animations
uint32_t ffx_sceneNode_isAnimating(FfxNode node);

void ffx_sceneNode_advanceAnimations(FfxNode node, uint32_t advance);

// Stop all current animations on a node
void ffx_sceneNode_stopAnimations(FfxNode node, bool completeAnimations);


///////////////////////////////
// Fill

FfxNode ffx_scene_createFill(FfxScene scene, color_ffxt color);

color_ffxt ffx_sceneFill_getColor(FfxNode node);
void ffx_sceneFill_setColor(FfxNode node, color_ffxt color);


///////////////////////////////
// Box

FfxNode ffx_scene_createBox(FfxScene scene, FfxSize size);

color_ffxt ffx_sceneBox_getColor(FfxNode node);
void ffx_sceneBox_setColor(FfxNode node, color_ffxt color);

FfxSize ffx_sceneBox_getSize(FfxNode node);
void ffx_sceneBox_setSize(FfxNode node, FfxSize size);


///////////////////////////////
// Label

FfxNode ffx_scene_createLabel(FfxScene scene, FfxFont font, const char* text,
  size_t length);

size_t ffx_sceneLabel_copyText(FfxNode node, char* output, size_t length);
void ffx_sceneLabel_setText(FfxNode node, const char* data, size_t length);

FfxFont ffx_sceneLabel_getFont(FfxNode node);
void ffx_sceneLabel_setFont(FfxNode node, FfxFont font);

color_ffxt ffx_sceneLabel_getTextColor(FfxNode node);
void ffx_sceneLabel_setTextColor(FfxNode node, color_ffxt color);

color_ffxt ffx_sceneLabel_getOutlineColor(FfxNode node);
void ffx_sceneLabel_setOutlineColor(FfxNode node, color_ffxt color);


///////////////////////////////
// Image

FfxNode ffx_scene_createImage(FfxScene scene, uint8_t *data, size_t length);

color_ffxt ffx_sceneImage_getTint(FfxNode node);
void ffx_sceneImage_setTint(FfxNode node, color_ffxt color);

uint8_t* ffx_sceneImage_getData(FfxNode node);
void ffx_sceneImage_setData(FfxNode node, const char* data, size_t length);



#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __FIREFLY_SCENE_H__ */
