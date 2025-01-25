#include <stddef.h>
#include <stdio.h>

#include "firefly-scene-private.h"

/*
static void _renderDark50(FfxPoint pos, FfxProperty a, FfxProperty b, uint16_t *frameBuffer, int32_t y0, int32_t height) {

    // The box is above the fragment or to the right of the display; skip
    if (pos.y >= y0 + height || pos.x >= 240) { return; }

    // Compute start y and height within the fragment
    int32_t sy = pos.y - y0;
    int32_t h = a.size.height;
    if (sy < 0) {
        h += sy;
        sy = 0;
    }
    if (h <= 0) { return; }

    // Compute the start x and width within the fragment
    int32_t sx = pos.x;
    int32_t w = a.size.width;
    if (sx < 0) {
        w += sx;
        sx = 0;
    }
    if (w <= 0) { return; }

    // Extends past the fragment bounds; shrink
    if (sy + h > height) { h = height - sy; }
    if (sx + w > 240) { w = 240 - sx; }

    for (uint32_t y = 0; y < h; y++) {
        uint16_t *output = &frameBuffer[240 * (sy + y) + sx];
        for (uint32_t x = 0; x < w; x++) {
            // (RRRR 0GGG G0BB BBB0) >> 1
            uint16_t darker = ((*output) & 0xf7be) >> 1;
            *output++ = darker;
        }
    }
}

static void _renderDark75(FfxPoint pos, FfxProperty a, FfxProperty b, uint16_t *frameBuffer, int32_t y0, int32_t height) {

    // The box is above the fragment or to the right of the display; skip
    if (pos.y >= y0 + height || pos.x >= 240) { return; }

    // Compute start y and height within the fragment
    int32_t sy = pos.y - y0;
    int32_t h = a.size.height;
    if (sy < 0) {
        h += sy;
        sy = 0;
    }
    if (h <= 0) { return; }

    // Compute the start x and width within the fragment
    int32_t sx = pos.x;
    int32_t w = a.size.width;
    if (sx < 0) {
        w += sx;
        sx = 0;
    }
    if (w <= 0) { return; }

    // Extends past the fragment bounds; shrink
    if (sy + h > height) { h = height - sy; }
    if (sx + w > 240) { w = 240 - sx; }

    for (uint32_t y = 0; y < h; y++) {
        uint16_t *output = &frameBuffer[240 * (sy + y) + sx];
        for (uint32_t x = 0; x < w; x++) {
            // (RRR0 0GGG G00B BB00) >> 2
            uint16_t darker = ((*output) & 0xe79c) >> 2;
            *output++ = darker;
        }
    }
}

static void _render(FfxPoint pos, FfxProperty a, FfxProperty b, uint16_t *frameBuffer, int32_t y0, int32_t height) {

    // The box is above the fragment or to the right of the display; skip
    if (pos.y >= y0 + height || pos.x >= 240) { return; }

    // Compute start y and height within the fragment
    int32_t sy = pos.y - y0;
    int32_t h = a.size.height;
    if (sy < 0) {
        h += sy;
        sy = 0;
    }
    if (h <= 0) { return; }

    // Compute the start x and width within the fragment
    int32_t sx = pos.x;
    int32_t w = a.size.width;
    if (sx < 0) {
        w += sx;
        sx = 0;
    }
    if (w <= 0) { return; }

    // Extends past the fragment bounds; shrink
    if (sy + h > height) { h = height - sy; }
    if (sx + w > 240) { w = 240 - sx; }

    rgb16_ffxt _color = ffx_color_rgb16(b.color);
    uint16_t color = _color & 0xffff;

    for (uint32_t y = 0; y < h; y++) {
        uint16_t *output = &frameBuffer[240 * (sy + y) + sx];
        for (uint32_t x = 0; x < w; x++) {
              *output++ = color;
        }
    }
}

static void _sequence(FfxScene scene, FfxPoint worldPos, FfxNode node) {
    color_ffxt *color = ffx_scene_boxColor(node);
    if (*color == RGB_DARK50) {
        ffx_scene_createRenderNode(scene, node, worldPos, _renderDark50);
    } else if (*color == RGB_DARK75) {
        ffx_scene_createRenderNode(scene, node, worldPos, _renderDark75);
    } else {
        ffx_scene_createRenderNode(scene, node, worldPos, _render);
    }
}
*/

/*
static void _animateColor(FfxNode node, fixed_ffxt t, FfxProperty p0, FfxProperty p1) {
    color_ffxt *color = ffx_scene_boxColor(node);
    *color = ffx_color_lerpfx(p0.color, p1.color, t);
}

uint32_t ffx_scene_boxAnimateColor(FfxScene scene, FfxNode node,
  color_ffxt target, uint32_t duration, FfxCurveFunc curve,
  FfxSceneAnimationCompletion onComplete) {

    FfxProperty *b = ffx_scene_nodePropertyB(node);

    FfxProperty start, end;
    start.color = b->color;
    end.color = target;

    FfxNode animate = ffx_scene_createAnimationNode(scene, node, start, end,
      duration, _animateColor, curve, onComplete);
    if (animate == NULL) { return 0; }

    return 1;
}
*/


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

static void destroyFunc(FfxNode node) {
}

static void sequenceFunc(FfxNode node, FfxPoint worldPos) {
    BoxNode *state = ffx_sceneNode_getState(node);

    FfxPoint pos = ffx_sceneNode_getPosition(node);
    pos.x += worldPos.x;
    pos.y += worldPos.y;

    BoxRender *render = ffx_scene_createRender(node, sizeof(BoxRender));
    render->size = state->size;
    render->color = state->color;
    render->position = pos;
}

static void renderFunc(void *_render, uint16_t *frameBuffer,
  FfxPoint origin, FfxSize size) {

    BoxRender *render = _render;

    FfxClip clip = ffx_scene_clip(render->position, render->size, origin,
      size);

    if (clip.width == 0) { return; }

    rgb16_ffxt _color = ffx_color_rgb16(render->color);
    uint16_t color = _color & 0xffff;

    for (uint32_t y = 0; y < clip.height; y++) {
        uint16_t *output = &frameBuffer[240 * (clip.vpY + y) + clip.vpX];
        for (uint32_t x = 0; x < clip.width; x++) {
              *output++ = color;
        }
    }
}

static void dumpFunc(FfxNode node, int indent) {
    FfxPoint pos = ffx_sceneNode_getPosition(node);

    BoxNode *state = ffx_sceneNode_getState(node);

    char colorName[COLOR_NAME_LENGTH] = { 0 };
    ffx_color_name(state->color, colorName, sizeof(colorName));

    for (int i = 0; i < indent; i++) { printf("  "); }
    printf("<Box pos=%dx%d size=%dx%d color=%s>\n", pos.x, pos.y,
      state->size.width, state->size.height, colorName);
}

static const FfxNodeVTable vtable = {
    .destroyFunc = destroyFunc,
    .sequenceFunc = sequenceFunc,
    .renderFunc = renderFunc,
    .dumpFunc = dumpFunc,
};


//////////////////////////
// Life-cycle

FfxNode ffx_scene_createBox(FfxScene scene, FfxSize size) {
    FfxNode node = ffx_scene_createNode(scene, &vtable, sizeof(BoxNode));

    BoxNode *state = ffx_sceneNode_getState(node);
    state->size = size;

    return node;
}


//////////////////////////
// Properties

color_ffxt ffx_sceneBox_getColor(FfxNode node) {
    BoxNode *state = ffx_sceneNode_getState(node);
    return state->color;
}

void ffx_sceneBox_setColor(FfxNode node, color_ffxt color) {
    BoxNode *state = ffx_sceneNode_getState(node);
    state->color = color;
}

FfxSize ffx_sceneBox_getSize(FfxNode node) {
    BoxNode *state = ffx_sceneNode_getState(node);
    return state->size;
}

void ffx_sceneBox_setSize(FfxNode node, FfxSize size) {
    BoxNode *state = ffx_sceneNode_getState(node);
    state->size = size;
}

