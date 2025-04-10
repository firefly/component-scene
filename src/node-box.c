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


//////////////////////////
// Methods

static bool walkFunc(FfxNode node, FfxNodeVisitFunc enterFunc,
  FfxNodeVisitFunc exitFunc, void* arg) {
    return true;
}

static void destroyFunc(FfxNode node) {
}

static void sequenceFunc(FfxNode node, FfxPoint worldPos) {
    BoxNode *box = ffx_sceneNode_getState(node);

    FfxPoint pos = ffx_sceneNode_getPosition(node);
    pos.x += worldPos.x;
    pos.y += worldPos.y;

    BoxRender *render = ffx_scene_createRender(node, sizeof(BoxRender));
    render->size = box->size;
    render->color = box->color;
    render->position = pos;
}

static void renderBoxBlend(uint16_t *frameBuffer, int32_t ox, int32_t oy,
  int32_t width, int32_t height, color_ffxt _color) {

    // Get the color an RGB565 components
    FfxColorRGB color = ffx_color_parseRGB(_color);
    int r = color.red >> 3;
    int g = color.green >> 2;
    int b = color.blue >> 3;

    // @TODO: Can I keep this as a fixed.26.6 and just >> 6?

    // Pad alpha with 11 zeros (i.e. 0x20 => 0x10000; 1 in fixed-point)
    fixed_ffxt alpha = color.opacity << 11;
    fixed_ffxt alpha_1 = FM_1 - alpha;

    for (uint32_t y = 0; y < height; y++) {
        uint16_t *output = &frameBuffer[240 * (oy + y) + ox];
        for (uint32_t x = 0; x < width; x++) {

            // Get the background RGB565 components
            uint16_t bg = *output;
            int bgR = bg >> 11;
            int bgG = (bg >> 5) & 0x3f;
            int bgB = bg & 0x1f;

            // Blend the values and convert from fixed-point
            int blendR = ((alpha * r) + (alpha_1 * bgR)) >> 16;
            int blendG = ((alpha * g) + (alpha_1 * bgG)) >> 16;
            int blendB = ((alpha * b) + (alpha_1 * bgB)) >> 16;

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

static void renderFunc(void *_render, uint16_t *frameBuffer,
  FfxPoint origin, FfxSize size) {

    BoxRender *render = _render;

    FfxClip clip = ffx_scene_clip(render->position, render->size, origin,
      size);

    uint8_t opacity = ffx_color_getOpacity(render->color);

    if (clip.width == 0 || opacity == 0) { return; }

    if (render->color == RGBA_DARKER50) {
        renderBoxDarker50(frameBuffer, clip.vpX, clip.vpY, clip.width,
          clip.height);
        return;
    }

    if (render->color == RGBA_DARKER75) {
        renderBoxDarker75(frameBuffer, clip.vpX, clip.vpY, clip.width,
          clip.height);
        return;
    }

    if (opacity == MAX_OPACITY) {
        renderBoxOpaque(frameBuffer, clip.vpX, clip.vpY, clip.width,
          clip.height, render->color);
        return;
    }

    renderBoxBlend(frameBuffer, clip.vpX, clip.vpY, clip.width, clip.height,
      render->color);
}

static void dumpFunc(FfxNode node, int indent) {
    FfxPoint pos = ffx_sceneNode_getPosition(node);

    BoxNode *box = ffx_sceneNode_getState(node);

    char colorName[COLOR_STRING_LENGTH] = { 0 };

    for (int i = 0; i < indent; i++) { printf("  "); }
    printf("<Box pos=%dx%d size=%dx%d color=%s>\n", pos.x, pos.y,
      box->size.width, box->size.height,
      ffx_color_sprintf(box->color, colorName));
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

FfxNode ffx_scene_createBox(FfxScene scene, FfxSize size) {
    FfxNode node = ffx_scene_createNode(scene, &vtable, sizeof(BoxNode));

    BoxNode *box = ffx_sceneNode_getState(node);
    box->size = size;

    return node;
}

bool ffx_scene_isBox(FfxNode node) {
    return ffx_scene_isNode(node, &vtable);
}


//////////////////////////
// Properties

color_ffxt ffx_sceneBox_getColor(FfxNode node) {
    BoxNode *box = ffx_sceneNode_getState(node);
    return box->color;
}

static void setColor(FfxNode node, color_ffxt color) {
    BoxNode *box = ffx_sceneNode_getState(node);
    box->color = color;
}

void ffx_sceneBox_setColor(FfxNode node, color_ffxt color) {
    BoxNode *box = ffx_sceneNode_getState(node);
    ffx_sceneNode_createColorAction(node, box->color, color, setColor);
}

FfxSize ffx_sceneBox_getSize(FfxNode node) {
    BoxNode *box = ffx_sceneNode_getState(node);
    return box->size;
}

static void setSize(FfxNode node, FfxSize size) {
    BoxNode *box = ffx_sceneNode_getState(node);
    box->size = size;
}

void ffx_sceneBox_setSize(FfxNode node, FfxSize size) {
    BoxNode *box = ffx_sceneNode_getState(node);
    ffx_sceneNode_createSizeAction(node, box->size, size, setSize);
}

