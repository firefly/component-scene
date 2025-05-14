#include <stdio.h>
#include <stddef.h>

#include "firefly-scene-private.h"
#include "firefly-fixed.h"


typedef struct ImageNode {
    const uint16_t *data;
    color_ffxt tint;
} ImageNode;

typedef struct ImageRender {
    FfxPoint position;
    const uint16_t *data;
    color_ffxt tint;
} ImageRender;


static bool walkFunc(FfxNode node, FfxNodeVisitFunc enterFunc,
  FfxNodeVisitFunc exitFunc, void* arg);
static void destroyFunc(FfxNode node);
static void sequenceFunc(FfxNode node, FfxPoint worldPos);
static void renderFunc(void *_render, uint16_t *_frameBuffer,
  FfxPoint origin, FfxSize size);
static void dumpFunc(FfxNode node, int indent);

static const char name[] = "ImageNode";
static const FfxNodeVTable vtable = {
    .walkFunc = walkFunc,
    .destroyFunc = destroyFunc,
    .sequenceFunc = sequenceFunc,
    .renderFunc = renderFunc,
    .dumpFunc = dumpFunc,
    .name = name
};


//////////////////////////
// Image Rasterizing

static  void _renderRGB565(ImageRender *render, uint16_t *frameBuffer,
  FfxPoint origin, FfxSize size) {

    const uint16_t *data = render->data;
    int16_t width = data[1];

    FfxClip clip = ffx_scene_clip(render->position, (FfxSize){
        .width = width, .height = data[2]
    }, origin, size);

    if (clip.width == 0) { return; }


    // Skip the header bytes
    data += 3;

    for (int32_t y = clip.height; y; y--) {
        uint16_t *output = &frameBuffer[(240 * (clip.vpY + y - 1)) + clip.vpX];
        const uint16_t *input = &data[((clip.y + y - 1) * width) + clip.x];
        for (int32_t x = clip.width; x; x--) {
            *output++ = *input++;
        }
    }
}

#define UFIXED_1_21_ONE       (0x200000)

static  void _renderRGB565_A4(ImageRender *render, uint16_t *frameBuffer,
  FfxPoint origin, FfxSize size) {

    const uint16_t *data = render->data;
    int16_t width = data[1];

    FfxClip clip = ffx_scene_clip(render->position, (FfxSize){
        .width = width, .height = data[2]
    }, origin, size);
    if (clip.width == 0) { return; }

    // Point to the alpha data
    const uint16_t *alpha = &data[3];
    uint16_t alphaCount = alpha[0];
    alpha++;

    // Point to the bitmap data (advance past the alpha data)
    data += alphaCount + 3 + 1;

    // Additional alpha (from tint) to apply; ufixed:1.5
    int32_t opacity = ffx_color_getOpacity(render->tint);

    for (int32_t y = clip.height; y; y--) {
        uint16_t *output = &frameBuffer[(240 * (clip.vpY + y - 1)) + clip.vpX];

        const uint16_t *input = &data[((clip.y + y - 1) * width) + clip.x];
        uint16_t ia = (((clip.y + y - 1) * width) + clip.x);

        for (int32_t x = clip.width; x; x--) {
            uint16_t fg = *input++;

            // Get the alpha (ufixed:1.16 * ufixed:1.5 => ufixed:1.21)
            uint32_t fga = FIXED_BITS_4((alpha[ia / 4] >> (12 - 4 *
              (ia % 4))) & 0x0f) * opacity;
            ia++;

            if (fga >= UFIXED_1_21_ONE) {
                // Fully opaque
                *output = fg;

            } else if (fga) {
                // Paritially translucent

                // Get the background RGB565 components
                uint16_t bg = *output;
                int bgR = bg >> 11;
                int bgG = (bg >> 5) & 0x3f;
                int bgB = bg & 0x1f;

                int fgR = fg >> 11;
                int fgG = (fg >> 5) & 0x3f;
                int fgB = fg & 0x1f;

                uint32_t fga_1 = UFIXED_1_21_ONE - fga;

                // Blend the values and convert from fixed-point
                int blendR = ((fga * fgR) + (fga_1 * bgR)) >> 21;
                int blendG = ((fga * fgG) + (fga_1 * bgG)) >> 21;
                int blendB = ((fga * fgB) + (fga_1 * bgB)) >> 21;

                *output = (blendR << 11) | (blendG << 5) | blendB;
            }

            output++;
        }
    }

}

static  void _renderPal8(ImageRender *render, uint16_t *frameBuffer,
  FfxPoint origin, FfxSize size) {

    const uint16_t *data = render->data;
    int16_t width = data[1];

    FfxClip clip = ffx_scene_clip(render->position, (FfxSize){
        .width = width, .height = data[2]
    }, origin, size);
    if (clip.width == 0) { return; }

    // Point to the palette data
    const uint16_t *palette = &data[3];

    const uint8_t *pixels = (uint8_t*)&data[3 + 256];

    // Point to the bitmap data (advance past the palette data)
    data += 3 + 256;

    for (int32_t y = clip.height; y; y--) {
        uint16_t *output = &frameBuffer[(240 * (clip.vpY + y - 1)) + clip.vpX];
        const uint8_t *input = &pixels[((clip.y + y - 1) * width) + clip.x];
        for (int32_t x = clip.width; x; x--) {
            *output++ = palette[*input++];
        }
    }
}
/*
static  void _imageRenderPal8(FfxPoint pos, FfxProperty a, FfxProperty b,
  uint16_t *frameBuffer, int32_t y0, int32_t height) {

    uint16_t *data = (uint16_t*)(a.ptr);

    uint16_t *palette = &data[3];
    uint8_t *pixels = (uint8_t*)&data[3 + 256];

    for (int32_t y = oh; y; y--) {
        uint16_t *output = &frameBuffer[(240 * (oy + y - 1)) + ox];
        uint8_t *input = &pixels[((iy + y - 1) * w) + ix];
        for (int32_t x = ow; x; x--) {
            *output++ = palette[*input++];
        }
    }
}
*/


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
    ImageNode *state = ffx_sceneNode_getState(node, &vtable);

    FfxPoint pos = ffx_sceneNode_getPosition(node);
    pos.x += worldPos.x;
    pos.y += worldPos.y;

    ImageRender *render = ffx_scene_createRender(node, sizeof(ImageRender));
    render->data = state->data;
    render->tint = state->tint;
    render->position = pos;
}

static void renderFunc(void *_render, uint16_t *frameBuffer,
  FfxPoint origin, FfxSize size) {

    ImageRender *render = _render;

    if ((render->data[0] & 0x0f) == 0x05) {
        _renderRGB565_A4(render, frameBuffer, origin, size);
    } else if ((render->data[0] & 0x0f) == 0x04) {
        _renderRGB565(render, frameBuffer, origin, size);
    } else if ((render->data[0] & 0xff) == 0x38) {
        _renderPal8(render, frameBuffer, origin, size);
    }

}

static void dumpFunc(FfxNode node, int indent) {
    FfxPoint pos = ffx_sceneNode_getPosition(node);

    ImageNode *state = ffx_sceneNode_getState(node, &vtable);

    FfxSize size = ffx_scene_getImageSize(state->data, 3);

    for (int i = 0; i < indent; i++) { printf("  "); }
    printf("<Image pos=%dx%d size=%dx%x image=%p>\n", pos.x, pos.y,
      size.width, size.height, state->data);
}


//////////////////////////
// Life-cycle

FfxNode ffx_scene_createImage(FfxScene scene, const uint16_t *data,
  size_t dataLength) {

    FfxSize size = ffx_scene_getImageSize(data, dataLength);
    if (size.width == 0 || size.height == 0) { return NULL; }

    FfxNode node = ffx_scene_createNode(scene, &vtable, sizeof(ImageNode));

    ImageNode *state = ffx_sceneNode_getState(node, &vtable);
    state->data = data;

    return node;
}

bool ffx_scene_isImage(FfxNode node) {
    return ffx_scene_isNode(node, &vtable);
}


//////////////////////////
// Properties

const uint16_t* ffx_sceneImage_getData(FfxNode node) {
    ImageNode *img = ffx_sceneNode_getState(node, &vtable);
    if (img == NULL) { return NULL; }
    return img->data;
}

void ffx_sceneImage_setData(FfxNode node, const uint16_t *data,
  size_t length) {
    ImageNode *img = ffx_sceneNode_getState(node, &vtable);
    if (img == NULL) { return; }

    FfxSize size = ffx_scene_getImageSize(data, length);
    if (size.width) {
        img->data = data;
    }
}

color_ffxt ffx_sceneImage_getTint(FfxNode node) {
    ImageNode *img = ffx_sceneNode_getState(node, &vtable);
    if (img == NULL) { return 0; }
    return img->tint;
}

static void setTint(FfxNode node, color_ffxt tint) {
    ImageNode *img = ffx_sceneNode_getState(node, &vtable);
    if (img == NULL) { return; }
    img->tint = tint;
}

void ffx_sceneImage_setTint(FfxNode node, color_ffxt tint) {
    ImageNode *img = ffx_sceneNode_getState(node, &vtable);
    if (img == NULL) { return; }
    ffx_sceneNode_createColorAction(node, img->tint, tint, setTint);
}


//////////////////////////
// Static Methods

FfxSize ffx_scene_getImageSize(const uint16_t *data, size_t length) {
    FfxSize size;
    size.width = data[1];
    size.height = data[2];

    // @TODO validate image data

    return size;
}

