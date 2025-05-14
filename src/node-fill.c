#include <stddef.h>
#include <stdio.h>

#include "firefly-scene-private.h"


typedef struct FillNode {
    color_ffxt color;
} FillNode;


static bool walkFunc(FfxNode node, FfxNodeVisitFunc enterFunc,
  FfxNodeVisitFunc exitFunc, void* arg);
static void destroyFunc(FfxNode node);
static void sequenceFunc(FfxNode node, FfxPoint worldPos);
static void renderFunc(void *_render, uint16_t *_frameBuffer,
  FfxPoint origin, FfxSize size);
static void dumpFunc(FfxNode node, int indent);

static const char name[] = "FillNode";
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

static void destroyFunc(FfxNode node) { }

static void sequenceFunc(FfxNode node, FfxPoint worldPos) {
    FillNode *fill = ffx_sceneNode_getState(node, &vtable);

    FillNode *render = ffx_scene_createRender(node, sizeof(FillNode));
    render->color = fill->color;
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
    FillNode *fill = ffx_sceneNode_getState(node, &vtable);

    char colorName[COLOR_STRING_LENGTH] = { 0 };

    for (int i = 0; i < indent; i++) { printf("  "); }
    printf("<Fill color=%s>\n", ffx_color_sprintf(fill->color, colorName));
}


//////////////////////////
// Life-cycle

FfxNode ffx_scene_createFill(FfxScene scene, color_ffxt color) {
    FfxNode node = ffx_scene_createNode(scene, &vtable, sizeof(FillNode));

    FillNode *fill = ffx_sceneNode_getState(node, &vtable);
    fill->color = color;

    return node;
}

bool ffx_scene_isFill(FfxNode node) {
    return ffx_scene_isNode(node, &vtable);
}


//////////////////////////
// Properties

color_ffxt ffx_sceneFill_getColor(FfxNode node) {
    FillNode *fill = ffx_sceneNode_getState(node, &vtable);
    if (fill == NULL) { return 0; }
    return fill->color;
}

static void setColor(FfxNode node, color_ffxt color) {
    FillNode *fill = ffx_sceneNode_getState(node, &vtable);
    if (fill == NULL) { return; }
    fill->color = color;
}

void ffx_sceneFill_setColor(FfxNode node, color_ffxt color) {
    FillNode *fill = ffx_sceneNode_getState(node, &vtable);
    if (fill == NULL) { return; }
    ffx_sceneNode_createColorAction(node, fill->color, color, setColor);
}
