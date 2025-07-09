#ifndef __FIREFLY_SCENE_H__
#define __FIREFLY_SCENE_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdbool.h>
#include <stdint.h>

#include "firefly-color.h"
#include "firefly-curves.h"


/**
 *  Scene object (opaque; do not inspect or rely on internals).
 */
typedef void* FfxScene;

/**
 *  Scene Node object (opaque; do not inspect or rely on internals)
 */
typedef void* FfxNode;

typedef int FfxNodeTag;

/**
 *  Point object.
 */
typedef struct FfxPoint {
    int16_t x;
    int16_t y;
} FfxPoint;


/**
 *  Size object.
 */
typedef struct FfxSize {
    uint16_t width;
    uint16_t height;
} FfxSize;


// Font enum definition
// [ 1 bit: isBold ] [ 1 bit: reserved ] [ 6 bits: size ]

/**
 *  Label font.
 */
typedef enum FfxFont {
    FfxFontLarge       = 0x18,    // 24-point
    FfxFontLargeBold   = 0x98,    // 24-point bold
    FfxFontMedium      = 0x14,    // 20-point
    FfxFontMediumBold  = 0x94,    // 20-point bold
    FfxFontSmall       = 0x0f,    // 15-point
    FfxFontSmallBold   = 0x8f,    // 15-point bold

    FfxFontSizeMask    = 0x3f,
    FfxFontBoldMask    = 0x80
} FfxFont;

/**
 *  Label text alignment.
 */
typedef enum FfxTextAlign {
    FfxTextAlignTop              = (1 << 0), // y at top of bound
    FfxTextAlignMiddle           = (1 << 1), // y at middle of bounds
    FfxTextAlignBottom           = (1 << 2), // y at bottom of bounds
    FfxTextAlignMiddleBaseline   = (1 << 3), // y at middle less baseline
    FfxTextAlignBaseline         = (1 << 4), // y at bottom less baseline

    FfxTextAlignLeft             = (1 << 5),  // x at left
    FfxTextAlignCenter           = (1 << 6),  // x at center
    FfxTextAlignRight            = (1 << 7),  // x at right
} FfxTextAlign;

/**
 *  Font metric calculations. See [[ffx_sceneLabel_getFontMetrics]].
 */
typedef struct FfxFontMetrics {
    FfxSize size;
    int8_t descent;
    uint8_t outlineWidth;
    uint8_t points;
    bool isBold;
} FfxFontMetrics;

/**
 *  QR Code Mode.
 */
typedef enum FfxQRMode {
    FfxQRModeNumeric          = 0,
    FfxQRModeAlphanumeric     = 1,
    FfxQRModeByte             = 2
} FfxQRMode;

/**
 *  QR Code Error Correction Level.
 */
typedef enum FfxQRCorrection {
    FfxQRCorrectionLow        = 0,
    FfxQRCorrectionMedium     = 1,
    FfxQRCorrectionQuartile   = 2,
    FfxQRCorrectionHigh       = 3
} FfxQRCorrection;

/**
 *  QR Code Metrics calculations.
 */
typedef struct FfxQRMetrics {
    uint16_t size;
    uint8_t version;
    FfxQRMode mode;
    FfxQRCorrection level;
} FfxQRMetrics;

typedef bool (*FfxNodeVisitFunc)(FfxNode node, void* arg);

///////////////////////////////
// Static Methods

/**
 *  Compute the font metrics for %%font%%.
 */
FfxFontMetrics ffx_scene_getFontMetrics(FfxFont font);

/**
 *  Compute the image dimensions for %%data%% (with %%length%%).
 */
FfxSize ffx_scene_getImageSize(const uint16_t *data, size_t length);

/**
 *  Compute the QR Code metrics for %%text%% with %%minLevel%%.
 *
 *  If the required version to store %%text%% has additional space for
 *  higher error correction levels, it will be automatically elevated.
 */
FfxQRMetrics ffx_scene_getQRMetrics(const char* text,
  FfxQRCorrection minLevel);

/**
 *  Compute the QR Code metrics for %%data%% with %%minLevel%%.
 *
 *  If the required version to store %%text%% has additional space for
 *  higher error correction levels, it will be automatically elevated.
 */
FfxQRMetrics ffx_scene_getQRMetricsData(const uint8_t* data, size_t length,
  FfxQRCorrection minLevel);


/**
 *  Create a point from %%x%% and %%y%%.
 */
FfxPoint ffx_point(int16_t x, int16_t y);

/**
 *  Create a size from %%width%% and %%height%%.
 */
FfxSize ffx_size(int16_t width, int16_t height);


///////////////////////////////
// Animation

// @TODO: rename this FfxNodeStopReason (add cancelled for removed actions?)
typedef enum FfxSceneActionStop {
    // Indicates the animation ran to completion as normal
    FfxSceneActionStopNormal      = 0,

    // Indicates the animation was stopped and left at its current value
    FfxSceneActionStopCurrent     = (1 << 1) | (0 << 0),

    // Indicates the animation was stopped and jumped to the end value
    FfxSceneActionStopFinal       = (1 << 1) | (1 << 0),
} FfxSceneActionStop;

/**
 *  Callback fucntion for animation completion.
 */
typedef void (*FfxNodeAnimationCompletionFunc)(FfxNode node,
  FfxSceneActionStop stopType, void *arg);

/**
 *  Node animation configuration, which can be set in the
 *  [[FfxSceneAnimationSetupFunc]] function
 */
typedef struct FfxNodeAnimation {
    uint32_t delay;                             // Default: 0
    uint32_t duration;                          // Default: 0
    FfxCurveFunc curve;                         // Default: Linear
    FfxNodeAnimationCompletionFunc onComplete;  // Default: NULL
    void* arg;                                  // Default: NULL
} FfxNodeAnimation;



///////////////////////////////
// Scene

/**
 *  Memory allocator function.
 */
typedef uint8_t* (*FfxSceneAllocFunc)(size_t length, void *initArg);

/**
 *  Memory deallocator function.
 */
typedef void (*FfxSceneFreeFunc)(uint8_t* pointer, void* initArg);

typedef void* (*FfxSceneAnimationSetupFunc)(FfxNode node,
  FfxNodeAnimation animation, void *initArg);

typedef void (*FfxSceneAnimationDispatchFunc)(void *setupArg,
  FfxNodeAnimationCompletionFunc callFunc, FfxNode node,
  FfxSceneActionStop stopType, void *arg, void *initArg);

typedef bool (*FfxSceneAnimationQueueFunc)(void *animation, void *initArg);
typedef void* (*FfxSceneAnimationDequeueFunc)(void *initArg);

typedef struct FfxSceneConfig {
    FfxSceneAllocFunc allocFunc;
    FfxSceneFreeFunc freeFunc;

    FfxSceneAnimationSetupFunc setupFunc;
    FfxSceneAnimationDispatchFunc dispatchFunc;

    FfxSceneAnimationQueueFunc queueFunc;
    FfxSceneAnimationDequeueFunc dequeueFunc;

    void *initArg;
} FfxSceneConfig;

/**
 *  Allocate and initialize a new Scene Graph, which will use the
 *  %%allocFunc%% and %%freeFunc%% memory allocators and use the
 *  %%setupFunc%% and %%dispatchFunc%% to coordinate completion
 *  callbacks, passing %%initArg%% to all callback functions.
 */
FfxScene ffx_scene_init(FfxSceneAllocFunc allocFunc,
  FfxSceneFreeFunc freeFunc, FfxSceneAnimationSetupFunc setupFunc,
  FfxSceneAnimationDispatchFunc dispatchFunc,
  void *initArg);

/**
 *  Free %%scene%%.
 */
void ffx_scene_free(FfxScene scene);

FfxNode ffx_scene_findAnchor(FfxScene scene, FfxNodeTag tag);

bool ffx_scene_walk(FfxScene scene, FfxNodeVisitFunc enterFunc,
  FfxNodeVisitFunc exitFunc, void *arg);

/**
 *  Dump the scene graph to the console for debugging purposes.
 */
void ffx_scene_dump(FfxScene scene);

void ffx_scene_dumpStats(FfxScene scene);

/**
 *  Create a point-in-time renderable snapshot of %%scene%%.
 */
void ffx_scene_sequence(FfxScene scene);

/**
 *  Render the most recent snapshot of %%scene%% for the %%fragment%%
 *  within the viewport given by %%origin%% and %%size%%.
 */
void ffx_scene_render(FfxScene scene, uint16_t *fragment, FfxPoint origin,
  FfxSize size);

/**
 *  Get the root node of %%scene%%.
 */
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
void ffx_sceneNode_remove(FfxNode node);

/**
 *  Get the scene that created %%node%%.
 */
FfxNode ffx_sceneNode_getScene(FfxNode node);


/**
 *  Get the node position.
 */
FfxPoint ffx_sceneNode_getPosition(FfxNode node);

/**
 *  Set the %%node%% position to %%pos%%. If currently within an
 *  animation setup, the node will animate to that position.
 */
void ffx_sceneNode_setPosition(FfxNode node, FfxPoint pos);

void ffx_sceneNode_offsetPosition(FfxNode node, FfxPoint offset);

bool ffx_sceneNode_getHidden(FfxNode node);
void ffx_sceneNode_setHidden(FfxNode node, bool hidden);

FfxNode ffx_sceneNode_findAnchor(FfxNode node, FfxNodeTag tag);

bool ffx_sceneNode_walk(FfxNode scene, FfxNodeVisitFunc enterFunc,
  FfxNodeVisitFunc exitFunc, void *arg);


///////////////////////////////
// Group Node

/**
 *  Create a group node.
 */
FfxNode ffx_scene_createGroup(FfxScene scene);
bool ffx_scene_isGroup(FfxScene scene);

FfxNode ffx_sceneGroup_getFirstChild(FfxNode node);
void ffx_sceneGroup_appendChild(FfxNode node, FfxNode child);


///////////////////////////////
// Node: Animations

/**
 *  A callback function to setup all animations on %%node%%, using the
 *  %%animation%% configuration with the %%arg%% passed to
 *  [[ffx_sceneNode_animate]].
 */
typedef void (*FfxNodeAnimationSetupFunc)(FfxNode node,
  FfxNodeAnimation *animation, void *arg);

/**
 *  Animate %%node%% with the %%setupFunc%% and %%arg%%.
 */
void ffx_sceneNode_animate(FfxNode node, FfxNodeAnimationSetupFunc setupFunc,
  void *arg);


void ffx_sceneNode_runAnimation(FfxNode node,
  FfxNodeAnimationSetupFunc setupFunc, void *arg, uint32_t delay,
  uint32_t duration, FfxCurveFunc curve,
  FfxNodeAnimationCompletionFunc onComplete, void* completeArg);

/**
 *  Animate %%node%% to the %%position%% after %%delay%% for %%duration%%,
 *  using the given %%curve%% calling %%onComplete%% with %%arg%% when done.
 */
void ffx_sceneNode_animatePosition(FfxNode node, FfxPoint position,
  uint32_t delay, uint32_t duration, FfxCurveFunc curve,
  FfxNodeAnimationCompletionFunc onComplete, void* arg);


/**
 *  Returns true if %%node%% is running any animations.
 */
uint32_t ffx_sceneNode_isAnimating(FfxNode node);

/**
 *  Advance animations for %%node%%, by %%advance%% duration.
 */
void ffx_sceneNode_advanceAnimations(FfxNode node, uint32_t advance);

/**
 *  Stop all animations on %%node%%.
 */
void ffx_sceneNode_stopAnimations(FfxNode node, bool completeAnimations);


///////////////////////////////
// Fill

/**
 *  Create a fill node which will fill the entire viewport with %%color%%.
 */
FfxNode ffx_scene_createFill(FfxScene scene, color_ffxt color);
bool ffx_scene_isFill(FfxNode node);

/**
 *  Get the current fill color.
 */
color_ffxt ffx_sceneFill_getColor(FfxNode node);

/**
 *  Set the fill %%color%%. This property can be **animated**.
 *
 *  @TODO: currently opacityis not supported
 */
void ffx_sceneFill_setColor(FfxNode node, color_ffxt color);


///////////////////////////////
// Box

/**
 *  Create a box node.
 */
FfxNode ffx_scene_createBox(FfxScene scene, FfxSize size);
bool ffx_scene_isBox(FfxNode node);

/**
 *  Get the box color.
 */
color_ffxt ffx_sceneBox_getColor(FfxNode node);

/**
 *  Set the box %%color%%. This property can be **animated**.
 */
void ffx_sceneBox_setColor(FfxNode node, color_ffxt color);

void ffx_sceneBox_setOpacity(FfxNode node, uint8_t opacity);

void ffx_sceneBox_animateColor(FfxNode node, color_ffxt color,
  uint32_t delay, uint32_t duration, FfxCurveFunc curve,
  FfxNodeAnimationCompletionFunc onComplete, void* arg);

void ffx_sceneBox_animateOpacity(FfxNode node, uint8_t opacity,
  uint32_t delay, uint32_t duration, FfxCurveFunc curve,
  FfxNodeAnimationCompletionFunc onComplete, void* arg);


/**
 *  Get the box size.
 */
FfxSize ffx_sceneBox_getSize(FfxNode node);

/**
 *  Set the box %%size%%. This property can be **animated**.
 */
void ffx_sceneBox_setSize(FfxNode node, FfxSize size);


///////////////////////////////
// Label

/**
 *  Create a label.
 */
FfxNode ffx_scene_createLabel(FfxScene scene, FfxFont font, const char* text);
bool ffx_scene_isLabel(FfxNode node);

size_t ffx_sceneLabel_getTextLength(FfxNode node);
size_t ffx_sceneLabel_copyText(FfxNode node, char* output, size_t length);

void ffx_sceneLabel_setText(FfxNode node, const char* text);
void ffx_sceneLabel_setTextFormat(FfxNode node, const char* format, ...);

void ffx_sceneLabel_appendText(FfxNode node, const char* text);
void ffx_sceneLabel_appendCharacter(FfxNode node, char chr);
void ffx_sceneLabel_appendFormat(FfxNode node, const char* format, ...);

void ffx_sceneLabel_insertText(FfxNode node, size_t offset, const char* text);
void ffx_sceneLabel_insertCharacter(FfxNode node, size_t offset, char chr);
void ffx_sceneLabel_insertFormat(FfxNode node, size_t offset,
  const char* format, ...);

void ffx_sceneLabel_snipText(FfxNode node, size_t offset, size_t length);

FfxTextAlign ffx_sceneLabel_getAlign(FfxNode node);
void ffx_sceneLabel_setAlign(FfxNode node, FfxTextAlign align);

FfxFont ffx_sceneLabel_getFont(FfxNode node);
void ffx_sceneLabel_setFont(FfxNode node, FfxFont font);

/**
 *  Get the label text color.
 */
color_ffxt ffx_sceneLabel_getTextColor(FfxNode node);

/**
 *  Set the label text %%colore%%. This property can be **animated**.
 */
void ffx_sceneLabel_setTextColor(FfxNode node, color_ffxt color);

void ffx_sceneLabel_setOpacity(FfxNode node, uint8_t opacity);

void ffx_sceneLabel_animateTextColor(FfxNode node, color_ffxt color,
  uint32_t delay, uint32_t duration, FfxCurveFunc curve,
  FfxNodeAnimationCompletionFunc onComplete, void* arg);

void ffx_sceneLabel_animateOpacity(FfxNode node, uint8_t opacity,
  uint32_t delay, uint32_t duration, FfxCurveFunc curve,
  FfxNodeAnimationCompletionFunc onComplete, void* arg);

/**
 *  Get the label outline color.
 */
color_ffxt ffx_sceneLabel_getOutlineColor(FfxNode node);

/**
 *  Set the label outline %%colore%%. This property can be **animated**.
 */
void ffx_sceneLabel_setOutlineColor(FfxNode node, color_ffxt color);


///////////////////////////////
// Image

FfxNode ffx_scene_createImage(FfxScene scene, const uint16_t *data,
  size_t length);
bool ffx_scene_isImage(FfxNode node);

color_ffxt ffx_sceneImage_getTint(FfxNode node);
void ffx_sceneImage_setTint(FfxNode node, color_ffxt color);

const uint16_t* ffx_sceneImage_getData(FfxNode node);
void ffx_sceneImage_setData(FfxNode node, const uint16_t* data, size_t length);


///////////////////////////////
// Anchor

FfxNode ffx_scene_createAnchor(FfxScene scene, FfxNodeTag tag,
  size_t dataSize, FfxNode child);
bool ffx_scene_isAnchor(FfxNode node);

FfxNodeTag ffx_sceneAnchor_getTag(FfxNode node);
void ffx_sceneAnchor_setTag(FfxNode node, FfxNodeTag tag);

FfxNode ffx_sceneAnchor_getChild(FfxNode node);
void* ffx_sceneAnchor_getData(FfxNode node);


///////////////////////////////
// QR Code

/**
 *  Create a new QR Code Node for %%text%% and the %%minLevel%% error
 *  correction.
 */
FfxNode ffx_scene_createQR(FfxScene scene, const char* text,
  FfxQRCorrection minLevel);

/**
 *  Create a new QR Code Node for %%data%% and the %%minLevel%% error
 *  correction.
 */
FfxNode ffx_scene_createQRData(FfxScene scene, const uint8_t* data,
  size_t length, FfxQRCorrection minLevel);

/**
 *  Get the size of the QR Code, in pixels, including the quiet zone and
 *  adjusting for the module size.
 */
uint16_t ffx_sceneQR_getSize(FfxNode node);

/**
 *  Get the QR Code version code.
 */
uint8_t ffx_sceneQR_getVersion(FfxNode node);

/**
 *  Get the number of pixels (width and height) each square module of
 *  the QR Code will occupy.
 *
 *  The default is 1.
 */
uint8_t ffx_sceneQR_getModuleSize(FfxNode node);

/**
 *  Set the number of pixels (width and height) each square module of
 *  the QR Code will occupy.
 *
 *  The default is 1.
 */
void ffx_sceneQR_setModuleSize(FfxNode node, uint8_t moduleSize);

/**
 *  Get the number of modules around the QR Code that will be rendered
 *  with the **backgroundColor** to assist scanners searching the image.
 *
 *  The default (and recommended value) is 4.
 */
uint8_t ffx_sceneQR_getQuietZone(FfxNode node);

/**
 *  Set the number of modules around the QR Code that will be rendered
 *  with the **backgroundColor** to assist scanners searching the image.
 *
 *  The default (and recommended value) is 4.
 */
void ffx_sceneQR_setQuietZone(FfxNode node, uint8_t quietZone);

/**
 *  Get the foreground color of each module.
 *
 *  The default is COLOR_BLACK.
 */
color_ffxt ffx_sceneQR_getForegroundColor(FfxNode node);

/**
 *  set the foreground color of each module.
 *
 *  The default is COLOR_BLACK.
 */
void ffx_sceneQR_setForegroundColor(FfxNode node, color_ffxt color);

/**
 *  Get the background color of the QR code, including the quiet zone.
 *
 *  The default is COLOR_WHITE.
 */
color_ffxt ffx_sceneQR_getBackgroundColor(FfxNode node);

/**
 *  Set the background color of the QR code, including the quiet zone.
 *
 *  The default is COLOR_WHITE.
 */
void ffx_sceneQR_setBackgroundColor(FfxNode node, color_ffxt color);



///////////////////////////////
// Viewport
/*
FfxNode ffx_scene_createViewport(FfxScene scene, FfxNode child);
bool ffx_scene_isViewport(FfxNode node);

FfxNode ffx_sceneViewport_getChild(FfxNode node);

FfxSize ffx_sceneViewport_getSize(FfxNode node);
void ffx_sceneViewport_setSize(FfxNode node, FfxSize size);

FfxPoint ffx_sceneViewport_getOffset(FfxNode node);
void ffx_sceneViewport_setOffset(FfxNode node, FfxPoint offset);
*/

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __FIREFLY_SCENE_H__ */
