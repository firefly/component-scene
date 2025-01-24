#include "firefly-scene-private.h"

FfxClip ffx_scene_clip(FfxPoint origin, FfxSize size, FfxPoint vpOrigin,
  FfxSize vpSize) {

    FfxClip result = { 0 };

    int32_t x = 0, y = 0;
    int32_t width = size.width, height = size.height;

    int32_t objX0 = origin.x, objY0 = origin.y;
    int32_t objX1 = origin.x + size.width;
    int32_t objY1 = origin.y + size.height;

    int32_t vpX0 = vpOrigin.x, vpY0 = vpOrigin.y;
    int32_t vpX1 = vpOrigin.x + vpSize.width;
    int32_t vpY1 = vpOrigin.y + vpSize.height;

    // The object is completely outside the viewport
    if (objX0 >= vpX1 || objX1 < vpX0) { return result; }
    if (objY0 >= vpY1 || objY1 < vpY0) { return result; }


    int32_t d = objX0 - vpX0;
    if (d < 0) {
        // Object needs clipping to the left edge of the viewport
        x -= d;
        width += d;
        vpX0 = 0;
    } else {
        // Viewport adjusted to left edge of object
        vpX0 = objX0;
    }

    // Object needs clipping to the right edge of the viewport
    d = vpX1 - objX1;
    if (d < 0) { width += d; }


    d = objY0 - vpY0;
    if (d < 0) {
        // Object needs clipping to the top edge of the viewport
        y -= d;
        height += d;
        vpY0 = 0;
    } else {
        // Viewport adjusted to the top edge of the object
        vpY0 = objY0;
    }

    // Object needs clipping to the bottom edge of the viewport
    d = vpY1 - objY1;
    if (d < 0) { height += d; }

    result.x = x;
    result.y = y;

    result.vpX = vpX0;
    result.vpY = vpY0;

    result.width = width;
    result.height = height;

    return result;
}
/*
FfxClip ffx_scene_clip_old(FfxPoint pos, FfxSize size, int32_t y0, int32_t height) {
    FfxClip result = { 0 };

    int32_t dstY = pos.y - y0;

    // The node is entirely below the the viewport; abort
    if (dstY >= height) { return result; }

    // The node is entirely to the right of the viewport; abort
    if (pox.x >= 240) { return result; }

    int32_t srcY = 0;

    // The input box is above the fragment, trim off some of the top
    int32_t adjustHeight = height;
    if (dstY < 0) {
      srcY -= offY;
      adjustHeight += offY;
      dstY = 0;
    }

    // The node is entirely above the viewport; abort
    if (adjustHeight <= 0) { return result; }

    int32_t srcX = 0;
    int32_t dstX = pos.x;
    if (dstX < 0) {
        srcX -= dstX;
        width += dstX;
        dstX = 0;
    }

    // The node is entirely to the left of the viewport; abort
    if (width <= 0) { return result; }

    // Extends past the fragment bounds; shrink
    if (dstY + adjustHeight > height) { adjustHeight = height - dstY; }
    if (dstX + width > 240) { width = 240 - dstX; }

    result.srcX = srcX;
    result.srcY = srcY;

    result.dstX = dstX;
    result.dstY = dstY;

    result.width = width;
    result.height = adjustHeight;

    return result;
}

uint32_t calcBounds(int32_t *_pix, int32_t *_piy, int32_t *_ox, int32_t *_oy, int32_t *_w, int32_t *_h, int32_t y0, int32_t height) {
    // The top edge is belog the fragment; skip
    int32_t py = *_piy;
    if (py >= y0 + height) { return 1; }

    // The left edge is to the right of the fragment; skip
    int32_t px = *_pix;
    if (px >= 240) { return 1; }

    // Compute start y and height within the fragment
    int32_t iy = 0;
    int32_t oy = py - y0;
    int32_t h = *_h;
    if (oy < 0) {
        // The input box is above the fragment, trim off some of the top
        iy -= oy;
        h += oy;
        oy = 0;
    }
    if (h <= 0) { return 1; }

    // Compute the start x and width within the fragment
    int32_t ix = 0;
    int32_t ox = px;
    int32_t w = *_w;
    if (ox < 0) {
        // The input box is to the left of the fragment, trim off some of the left box
        ix -= ox;
        w += ox;
        ox = 0;
    }
    if (w <= 0) { return 1; }

    // Extends past the fragment bounds; shrink
    if (oy + h > height) { h = height - oy; }
    if (ox + w > 240) { w = 240 - ox; }

    // Copy the results back to the caller
    (*_pix) = ix;
    (*_piy) = iy;
    (*_ox) = ox;
    (*_oy) = oy;
    (*_w) = w;
    (*_h) = h;

    return 0;
}
*/
