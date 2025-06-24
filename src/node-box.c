#include <stddef.h>
#include <stdio.h>

#include "firefly-scene-private.h"


typedef struct BoxNode {
    FfxSize size;
    color_ffxt color;
} BoxNode;

typedef struct BoxRender {
    FfxPoint position;
    FfxSize size;
    color_ffxt color;
} BoxRender;


static bool walkFunc(FfxNode node, FfxNodeVisitFunc enterFunc,
  FfxNodeVisitFunc exitFunc, void* arg);
static void destroyFunc(FfxNode node);
static void sequenceFunc(FfxNode node, FfxPoint worldPos);
static void renderFunc(void *_render, uint16_t *_frameBuffer,
  FfxPoint origin, FfxSize size);
static void dumpFunc(FfxNode node, int indent);

static const char name[] = "BoxNode";
static const FfxNodeVTable vtable = {
    .walkFunc = walkFunc,
    .destroyFunc = destroyFunc,
    .sequenceFunc = sequenceFunc,
    .renderFunc = renderFunc,
    .dumpFunc = dumpFunc,
    .name = name
};


//////////////////////////
// Methods

static bool walkFunc(FfxNode node, FfxNodeVisitFunc enterFunc,
  FfxNodeVisitFunc exitFunc, void* arg) {

    if (enterFunc && !enterFunc(node, arg)) { return false; }
    if (exitFunc && !exitFunc(node, arg)) { return false; }
    return true;
}

static void destroyFunc(FfxNode node) {
}

static void sequenceFunc(FfxNode node, FfxPoint worldPos) {
    FfxPoint pos = ffx_sceneNode_getPosition(node);
    pos.x += worldPos.x;
    pos.y += worldPos.y;

    if (pos.x >= 240 || pos.y >= 240) { return; }

    BoxNode *box = ffx_sceneNode_getState(node, &vtable);

    if (pos.x + box->size.width < 0 || pos.y + box->size.height < 0 ||
      ffx_color_isTransparent(box->color)) { return; }

    BoxRender *render = ffx_scene_createRender(node, sizeof(BoxRender));
    render->size = box->size;
    render->color = box->color;
    render->position = pos;
}

static void renderBoxBlend(uint16_t *frameBuffer, int32_t ox, int32_t oy,
  int32_t width, int32_t height, color_ffxt _color) {

    FfxColorRGB color = ffx_color_parseRGB(_color);

    // Pad alpha with 11 zeros (i.e. 0x20 => 0x10000; 1 in fixed-point)
    fixed_ffxt alpha = color.opacity << 11;
    fixed_ffxt alpha_1 = FM_1 - alpha;

    // Get the premultiplied color components as fixed values
    int r = (color.red * alpha) >> 3;
    int g = (color.green * alpha) >> 2;
    int b = (color.blue * alpha) >> 3;

    for (uint32_t y = 0; y < height; y++) {
        uint16_t *output = &frameBuffer[240 * (oy + y) + ox];
        for (uint32_t x = 0; x < width; x++) {

            // Get the background RGB565 components
            uint16_t bg = *output;
            int bgR = bg >> 11;
            int bgG = (bg >> 5) & 0x3f;
            int bgB = bg & 0x1f;

            // Blend the values and convert from fixed-point
            int blendR = (r + (alpha_1 * bgR)) >> 16;
            int blendG = (g + (alpha_1 * bgG)) >> 16;
            int blendB = (b + (alpha_1 * bgB)) >> 16;

            *output++ = (blendR << 11) | (blendG << 5) | blendB;
        }
    }
}

static void renderBoxDarker50(uint16_t *frameBuffer, int32_t ox, int32_t oy,
  int32_t width, int32_t height) {

    for (uint32_t y = 0; y < height; y++) {
        uint16_t *output = &frameBuffer[240 * (oy + y) + ox];
        for (uint32_t x = 0; x < width; x++) {
            // (RRRR 0GGG G0BB BBB0) >> 1
            uint16_t darker = ((*output) & 0xf7be) >> 1;
            *output++ = darker;
        }
    }
}

static void renderBoxDarker75(uint16_t *frameBuffer, int32_t ox, int32_t oy,
  int32_t width, int32_t height) {

    for (uint32_t y = 0; y < height; y++) {
        uint16_t *output = &frameBuffer[240 * (oy + y) + ox];
        for (uint32_t x = 0; x < width; x++) {
            // (RRR0 0GGG G00B BB00) >> 2
            uint16_t darker = ((*output) & 0xe79c) >> 2;
            *output++ = darker;
        }
    }
}

static void renderBoxOpaque(uint16_t *frameBuffer, int32_t ox, int32_t oy,
  int32_t width, int32_t height, color_ffxt _color) {

    uint16_t color = ffx_color_rgb16(_color) & 0xffff;

    for (uint32_t y = 0; y < height; y++) {
        uint16_t *output = &frameBuffer[240 * (oy + y) + ox];
        for (uint32_t x = 0; x < width; x++) {
            *output++ = color;
        }
    }
}

void _ffx_renderBox(uint16_t *frameBuffer, int32_t ox, int32_t oy,
  int32_t width, int32_t height, color_ffxt color) {

    if (color == RGBA_DARKER50) {
        renderBoxDarker50(frameBuffer, ox, oy, width, height);
        return;
    }

    if (color == RGBA_DARKER75) {
        renderBoxDarker75(frameBuffer, ox, oy, width, height);
        return;
    }

    if (ffx_color_getOpacity(color) == MAX_OPACITY) {
        renderBoxOpaque(frameBuffer, ox, oy, width, height, color);
        return;
    }

    renderBoxBlend(frameBuffer, ox, oy, width, height, color);
}

static void renderFunc(void *_render, uint16_t *frameBuffer,
  FfxPoint origin, FfxSize size) {

    BoxRender *render = _render;

    FfxClip clip = ffx_scene_clip(render->position, render->size, origin,
      size);

    if (clip.width == 0) { return; }

    _ffx_renderBox(frameBuffer, clip.vpX, clip.vpY, clip.width, clip.height,
        render->color);
}

static void dumpFunc(FfxNode node, int indent) {
    FfxPoint pos = ffx_sceneNode_getPosition(node);

    BoxNode *box = ffx_sceneNode_getState(node, &vtable);

    char colorName[COLOR_STRING_LENGTH] = { 0 };

    for (int i = 0; i < indent; i++) { printf("  "); }
    printf("<Box pos=%dx%d size=%dx%d color=%s>\n", pos.x, pos.y,
      box->size.width, box->size.height,
      ffx_color_sprintf(box->color, colorName));
}


//////////////////////////
// Life-cycle

FfxNode ffx_scene_createBox(FfxScene scene, FfxSize size) {
    FfxNode node = ffx_scene_createNode(scene, &vtable, sizeof(BoxNode));

    BoxNode *box = ffx_sceneNode_getState(node, &vtable);
    box->size = size;

    return node;
}

bool ffx_scene_isBox(FfxNode node) {
    return ffx_scene_isNode(node, &vtable);
}


//////////////////////////
// Properties

color_ffxt ffx_sceneBox_getColor(FfxNode node) {
    BoxNode *box = ffx_sceneNode_getState(node, &vtable);
    if (box == NULL) { return 0; }
    return box->color;
}

static void setColor(FfxNode node, color_ffxt color) {
    BoxNode *box = ffx_sceneNode_getState(node, &vtable);
    if (box == NULL) { return; }
    box->color = color;
}

void ffx_sceneBox_setColor(FfxNode node, color_ffxt color) {
    BoxNode *box = ffx_sceneNode_getState(node, &vtable);
    if (box == NULL) { return; }
    ffx_sceneNode_createColorAction(node, box->color, color, setColor);
}

void ffx_sceneBox_setOpacity(FfxNode node, uint8_t opacity) {
    BoxNode *box = ffx_sceneNode_getState(node, &vtable);
    if (box == NULL) { return; }
    ffx_sceneBox_setColor(node, ffx_color_setOpacity(box->color, opacity));
}

FfxSize ffx_sceneBox_getSize(FfxNode node) {
    BoxNode *box = ffx_sceneNode_getState(node, &vtable);
    if (box == NULL) { return (FfxSize){ }; }
    return box->size;
}

static void setSize(FfxNode node, FfxSize size) {
    BoxNode *box = ffx_sceneNode_getState(node, &vtable);
    if (box == NULL) { return; }
    box->size = size;
}

void ffx_sceneBox_setSize(FfxNode node, FfxSize size) {
    BoxNode *box = ffx_sceneNode_getState(node, &vtable);
    if (box == NULL) { return; }
    ffx_sceneNode_createSizeAction(node, box->size, size, setSize);
}

//////////////////////////
// Animators

static void animateColor(FfxNode node, FfxNodeAnimation *animation,
  void *arg) {
    color_ffxt *color = arg;
    ffx_sceneBox_setColor(node, *color);
}

void ffx_sceneBox_animateColor(FfxNode node, color_ffxt color,
  uint32_t delay, uint32_t duration, FfxCurveFunc curve,
  FfxNodeAnimationCompletionFunc onComplete, void* arg) {

    BoxNode *box = ffx_sceneNode_getState(node, &vtable);
    if (box == NULL) { return; }
    if (box->color == color) { return; }

    ffx_sceneNode_runAnimation(node, animateColor, &color, delay, duration,
      curve, onComplete, arg);
}

void ffx_sceneBox_animateOpacity(FfxNode node, uint8_t opacity,
  uint32_t delay, uint32_t duration, FfxCurveFunc curve,
  FfxNodeAnimationCompletionFunc onComplete, void* arg) {

    BoxNode *box = ffx_sceneNode_getState(node, &vtable);
    if (box == NULL) { return; }
    color_ffxt color = ffx_color_setOpacity(box->color, opacity);

    ffx_sceneBox_animateColor(node, color, delay, duration, curve,
      onComplete, arg);
}
