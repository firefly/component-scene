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
        vpX0 = d;
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
        vpY0 = d;
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
