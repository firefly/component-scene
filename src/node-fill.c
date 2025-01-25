#include <stddef.h>
#include <stdio.h>

#include "firefly-scene-private.h"


typedef struct FillNode {
    color_ffxt color;
} FillNode;


//////////////////////////
// Methods

static void destroyFunc(FfxNode node) { }

static void sequenceFunc(FfxNode node, FfxPoint worldPos) {
    FillNode *state = ffx_sceneNode_getState(node);

    FillNode *render = ffx_scene_createRender(node, sizeof(FillNode));
    render->color = state->color;
}

static void renderFunc(void *_render, uint16_t *_frameBuffer,
  FfxPoint origin, FfxSize size) {

    FillNode *render = _render;

    uint32_t c = ffx_color_rgb16(render->color);
    c = ((c << 16) | c);

    uint32_t *frameBuffer = (uint32_t*)_frameBuffer;

    // @TODO: this doesn't work in general anymore; render line-by-line

    uint32_t count = size.width * size.height / 2;

    for (uint32_t i = 0; i < count; i++) { *frameBuffer++ = c; }
}

static void dumpFunc(FfxNode node, int indent) {
    FillNode *state = ffx_sceneNode_getState(node);

    char colorName[COLOR_NAME_LENGTH] = { 0 };
    ffx_color_name(state->color, colorName, sizeof(colorName));

    for (int i = 0; i < indent; i++) { printf("  "); }
    printf("<Fill color=%s>\n", colorName);
}

static const FfxNodeVTable vtable = {
    .destroyFunc = destroyFunc,
    .sequenceFunc = sequenceFunc,
    .renderFunc = renderFunc,
    .dumpFunc = dumpFunc,
};


//////////////////////////
// Life-cycle

FfxNode ffx_scene_createFill(FfxScene scene, color_ffxt color) {
    FfxNode node = ffx_scene_createNode(scene, &vtable, sizeof(FillNode));

    FillNode *state = ffx_sceneNode_getState(node);
    state->color = color;

    return node;
}


//////////////////////////
// Properties

color_ffxt ffx_sceneFill_getColor(FfxNode node) {
    FillNode *state = ffx_sceneNode_getState(node);
    return state->color;
}

void ffx_sceneFill_setColor(FfxNode node, color_ffxt color) {
    FillNode *state = ffx_sceneNode_getState(node);
    state->color = color;
}
