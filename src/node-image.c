/**
 *  Scene Node: Image
 *
 *  The image data formats supported include a header which indicates
 *  which specialized RenderFun should be used.
 *
 *  a:uint8_t* - a pointer the image data
 *  b:Size     - this value depends on the image type, and may
 *               represent multiple things, such as alpha or a
 *               mask value, pallette information or
 *               pre-computed data
 */

#include <stdio.h>

#include <stddef.h>

#include "firefly-scene-private.h"
#include "firefly-fixed.h"


static  void _imageRenderRGB565(FfxPoint pos, FfxProperty a, FfxProperty b,
  uint16_t *frameBuffer, int32_t y0, int32_t height) {

    uint16_t *data = (uint16_t*)(a.ptr);
    

    // printf("SS: %d %d", b.size.width, b.size.height);

    // @TODO: Move quick checks to _sequence?

    // The box is above the fragment or to the right of the display; skip
    if (pos.y >= y0 + height || pos.x >= 240) { return; }

    // Compute start y and height within the fragment
    int32_t iy = 0;
    int32_t oy = pos.y - y0;
    //int32_t oh = b.size.height;
    int32_t oh = data[2];
    if (oy < 0) {
        iy -= oy;
        oh += oy;
        oy = 0;
    }
    if (oh <= 0) { return; }

    // Compute the start x and width within the fragment
    int32_t ix = 0;
    int32_t ox = pos.x;
    //const int32_t w = b.size.width;
    const int32_t w = data[1];
    int32_t ow = w;
    if (ox < 0) {
        ix -= ox;
        ow += ox;
        ox = 0;
    }
    if (ow <= 0) { return; }

    // Extends past the fragment bounds; shrink
    if (oy + oh > height) { oh = height - oy; }
    if (ox + ow > 240) { ow = 240 - ox; }

    // Skip the header bytes
    //ix += 3;
    data += 3;

    for (int32_t y = oh; y; y--) {
        uint16_t *output = &frameBuffer[(240 * (oy + y - 1)) + ox];
        uint16_t *input = &data[((iy + y - 1) * w) + ix];
        for (int32_t x = ow; x; x--) {
            *output++ = *input++;
        }
    }
}

static  void _imageRenderRGBA5654(FfxPoint pos, FfxProperty a, FfxProperty b,
  uint16_t *frameBuffer, int32_t y0, int32_t height) {
    // The box is above the fragment or to the right of the display; skip
    if (pos.y >= y0 + height || pos.x >= 240) { return; }

    uint16_t *data = (uint16_t*)(a.ptr);

    // Compute start y and height within the fragment
    int32_t iy = 0;
    int32_t oy = pos.y - y0;
    int32_t oh = data[2];
    if (oy < 0) {
        iy -= oy;
        oh += oy;
        oy = 0;
    }
    if (oh <= 0) { return; }

    // Compute the start x and width within the fragment
    int32_t ix = 0;
    int32_t ox = pos.x;
    const int32_t w = data[1];
    int32_t ow = w;
    if (ox < 0) {
        ix -= ox;
        ow += ox;
        ox = 0;
    }
    if (ow <= 0) { return; }

    // Extends past the fragment bounds; shrink
    if (oy + oh > height) { oh = height - oy; }
    if (ox + ow > 240) { ow = 240 - ox; }

    // Skip the header bytes
    //ix += 3;

    uint16_t *alpha = (uint16_t*)(a.ptr);
    alpha += 3;
    uint16_t alphaCount = alpha[0];
    alpha++;

    //uint16_t *data = (uint16_t*)(a.ptr);
    data += alphaCount + 3;

    int32_t ga = (b.color >> 24);

    // @TODO: Lots of optimizations to be had here; use mul instead of div

    for (int32_t y = oh; y; y--) {
        uint16_t *output = &frameBuffer[(240 * (oy + y - 1)) + ox];
        uint16_t *input = &data[((iy + y - 1) * w) + ix];
        uint16_t ia = (((iy + y - 1) * w) + ix);
        for (int32_t x = ow; x; x--) {
            uint16_t fg = *input++;
            uint16_t bg = *output;

            uint16_t a = (alpha[ia / 4] >> (12 - 4 * (ia % 4))) & 0x0f;
            ia++;

            int32_t mulA = (a << 16) / 15;
            mulA = (mulA * ga) >> 5;

            if (mulA == 0x10000) {
                *output = fg;

            } else if (mulA != 0) {
                int32_t mulAi = 0x10000 - mulA;

                int32_t bgR = (bg >> 11) & 0x1f;
                int32_t bgG = (bg >> 5) & 0x3f;
                int32_t bgB = (bg >> 0) & 0x1f;

                int32_t fgR = (fg >> 11) & 0x1f;
                int32_t fgG = (fg >> 5) & 0x3f;
                int32_t fgB = (fg >> 0) & 0x1f;

                int blendR = ((mulAi * bgR) + (mulA * fgR)) >> 16;
                int blendG = ((mulAi * bgG) + (mulA * fgG)) >> 16;
                int blendB = ((mulAi * bgB) + (mulA * fgB)) >> 16;

                *output = (blendR << 11) | (blendG << 5) | blendB;
            }

            output++;
        }
    }
}

static void _imageSequenceRGB565(FfxScene scene, FfxPoint worldPos, FfxNode node) {
    ffx_scene_createRenderNode(scene, node, worldPos, _imageRenderRGB565);
}

static void _imageSequenceRGBA5654(FfxScene scene, FfxPoint worldPos, FfxNode node) {
    // 100% transparent
    color_ffxt *color = ffx_scene_imageColor(node);
    int32_t ga = (*color >> 24);
    if (ga == 0) { return; }

    ffx_scene_createRenderNode(scene, node, worldPos, _imageRenderRGBA5654);
}

static FfxSize _imageValidate(const uint16_t *data, size_t dataLength) {
    FfxSize size;
    size.width = data[1];
    size.height = data[2];
    // @TODO validate image data

    return size;
}

FfxNode ffx_scene_createImage(FfxScene scene, const uint16_t *data,
  size_t dataLength) {
    //_Scene* scene = _scene;

    FfxSize size = _imageValidate(data, dataLength);
    if (size.width == 0 || size.height == 0) { return NULL; }

    FfxProperty a, b;
    a.ptr = (void*)data;

    b.color = ffx_color_rgb(0, 0, 0, 0);

    if ((data[0] & 0x0f) == 0x05) {
        return ffx_scene_createNode(scene, _imageSequenceRGBA5654, a, b);
    } else if ((data[0] & 0x0f) == 0x04) {
        return ffx_scene_createNode(scene, _imageSequenceRGB565, a, b);
    }

    return NULL;


    //_Node* node = _addNode(scene, PointZero);
    //if (node == NULL) { return NULL; }

    //node->func.sequenceFunc = _imageSequence;

    //node->a.ptr = data;
    //node->b.size = size;

    //return node;
}

uint16_t* ffx_scene_imageData(FfxNode node) {
    FfxProperty *a = ffx_scene_nodePropertyA(node);
    return a->ptr;
}

color_ffxt* ffx_scene_imageColor(FfxNode node) {
    FfxProperty *b = ffx_scene_nodePropertyB(node);
    return &b->color;
}

/*
FfxSize ffx_scene_imageSize(FfxNode node) {
    if (node == NULL) { return SizeZero; }

    uint16_t *data = (uint16_t*)(a.ptr);

    FfxSize size;
    size.width = data[1];
    size.height = data[2];

    return size;
}
*/

static void _nodeAnimateAlpha(FfxNode node, fixed_ffxt t, FfxProperty p0, FfxProperty p1) {
    color_ffxt *color = ffx_scene_imageColor(node);
    int32_t c0 = p0.color >> 24, c1 = p1.color >> 24;

    *color = ffx_color_rgb(0, 0, 0, c0 + scalarfx(c1 - c0, t));
}

uint32_t ffx_scene_imageAnimateAlpha(FfxScene scene, FfxNode node, uint32_t target, uint32_t duration, FfxCurveFunc curve, FfxSceneAnimationCompletion onComplete) {
    color_ffxt *color = ffx_scene_imageColor(node);

    FfxProperty start, end;
    start.color = *color;
    end.color = ffx_color_rgb(0, 0, 0, target);

    FfxNode animate = ffx_scene_createAnimationNode(scene, node, start, end,
      duration, _nodeAnimateAlpha, curve, onComplete);
    if (animate == NULL) { return 0; }

    return 1;
}
