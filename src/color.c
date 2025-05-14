#include <stdbool.h>
#include <stdio.h>

#include "firefly-color.h"

// Color Format:
//  - RGBA = 00AA AAAA  RRRR RRRR  GGGG GGGG  BBBB BBBB
//  - HSVA = 01AA AAAA  HHHH HHHH  HHHH SSSS  SSVV VVVV
//  - Alpha is 0 for fully opaque and 0x20 for fully transparent, so that
//    a default value of 0 is fully opaque.


#define COLOR_HSV          (0x40000000)
#define MASK_ALPHA         (0x3f000000)


static uint32_t _getR(color_ffxt c) { return (c >> 16) & 0xff; }
static uint32_t _getG(color_ffxt c) { return (c >> 8) & 0xff; }
static uint32_t _getB(color_ffxt c) { return (c >> 0) & 0xff; }

static uint32_t _getH(color_ffxt c) { return (c >> 12) & 0xfff; }
static uint32_t _getS(color_ffxt c) { return (c >> 6) & 0x3f; }
static uint32_t _getV(color_ffxt c) { return (c >> 0) & 0x3f; }

static uint32_t _getO(color_ffxt c) { return 0x20 - ((c >> 24) & 0x3f); }


static uint32_t min(uint32_t a, uint32_t b) { return (a < b) ? a: b; }
static uint32_t max(uint32_t a, uint32_t b) { return (a > b) ? a: b; }

#define CLAMP(v,m)     (((v) <= (m)) ? (v): (m))


color_ffxt ffx_color_rgba(int32_t r, int32_t g, int32_t b, int32_t opacity) {
    color_ffxt result = 0;

    result |= CLAMP(r, 0xff) << 16;
    result |= CLAMP(g, 0xff) << 8;
    result |= CLAMP(b, 0xff);
    result |= (MAX_OPACITY - CLAMP(opacity, MAX_OPACITY)) << 24;

    return result;
}

color_ffxt ffx_color_rgb(int32_t r, int32_t g, int32_t b) {
    return ffx_color_rgba(r, g, b, MAX_OPACITY);
}

color_ffxt ffx_color_hsva(int32_t h, int32_t s, int32_t v, int32_t opacity) {
    color_ffxt result = COLOR_HSV;

    result |= (h % 3960) << 12;
    result |= CLAMP(s, 0x3f) << 6;
    result |= CLAMP(v, 0x3f);
    result |= (MAX_OPACITY - CLAMP(opacity, MAX_OPACITY)) << 24;

    return result;
}

color_ffxt ffx_color_hsv(int32_t h, int32_t s, int32_t v) {
    return ffx_color_hsva(h, s, v, MAX_OPACITY);
}

color_ffxt ffx_color_gray(uint8_t lum) {
    return ffx_color_rgba(lum, lum, lum, MAX_OPACITY);
}

#define B6        (0x3f)
#define B8        (0xff)
#define B11      (0x7ff)
#define B17    (0x1ffff)

#define B11_6    (0x7ff / 6)

// @TODO: This an be optimized using only multiply and keeping the
//        result in a fixed notation, like fixed 12:20.

// Heavily inspired by: https://stackoverflow.com/questions/3018313/algorithm-to-convert-rgb-to-hsv-and-hsv-to-rgb-in-range-0-255-for-both
static color_ffxt _fromHSV(color_ffxt color) {

    // Mask out the HSV-specific bits; keep the alpha
    color_ffxt result = color & MASK_ALPHA;

    // Normalized to [0, 63] (6-bits)
    int32_t s = _getS(color);

    // Normalize v to [0, 255] (8-bits)
    int32_t v = (_getV(color) * B8) / B6;

    if (s == 0) {
        result |= (v << 16) | (v << 8) | v;
        return result;
    }

    // Normalize s to [0, 2047] (11-bits); which has an error of 0.1666
    int32_t h = ((_getH(color) % 360) * B11) / 360;

    // Normalized to [0, 2047] (11-bits)
    int32_t region = h / B11_6;
    int32_t rem = (h - (region * B11_6)) * 6;

    int32_t p = (v * (B6 - s)) >> 6;
    int32_t t = (v * (B17 - (s * (B11 - rem)))) >> 17;
    int32_t q = (v * (B17 - (s * rem))) >> 17;

    // Map the RGB value based on the region
    switch (region) {
        case 0:
            result |= (v << 16) | (t << 8) | p;
            break;
        case 1:
            result |= (q << 16) | (v << 8) | p;
            break;
        case 2:
            result |= (p << 16) | (v << 8) | t;
            break;
        case 3:
            result |= (p << 16) | (q << 8) | v;
            break;
        case 4:
            result |= (t << 16) | (p << 8) | v;
            break;
        case 5:
            result |= (v << 16) | (p << 8) | q;
            break;
    }

    //printf("h=%ld s=%ld v=%ld region=%ld rem=%ld p=%ld qt=%ld\n", h, s, v, region, rem, p, qt);

    return result;
}


static color_ffxt _fromRGB(color_ffxt color) {
    int32_t r = _getR(color), g = _getG(color), b = _getB(color);
    uint32_t rgbMin = min(r, min(g, b));
    uint32_t rgbMax = max(r, max(g, b));

    // Mask out the RGB-specific bits; keep the alpha and set to HSV
    color_ffxt result = COLOR_HSV | (color & MASK_ALPHA);

    int32_t v = rgbMax;
    if (v == 0) { return result; }

    int32_t rgbDelta = rgbMax - rgbMin;

    int32_t s = 255 * rgbDelta / v;
    if (s == 0) { return result; }

    result |= (s & 0xfc) << 4;
    result |= (v >> 2);;

    if (rgbMax == r) {
        result |= (0 + 60 * (g - b) / rgbDelta) << 12;
    } else if (rgbMax == g) {
        result |= (120 + 60 * (g - b) / rgbDelta) << 12;
    } else {
        result |= (240 + 60 * (g - b) / rgbDelta) << 12;
    }

    return result;
}

rgb16_ffxt ffx_color_rgb16(color_ffxt color) {
    if (color & COLOR_HSV) { color = _fromHSV(color); }
    return RGB16((color >> 16) & 0xff, (color >> 8) & 0xff, color & 0xff);
}

rgb24_ffxt ffx_color_rgb24(color_ffxt color) {
    if (color & COLOR_HSV) { color = _fromHSV(color); }
    return color & 0x00ffffff;
}

rgba24_ffxt ffx_color_rgba24(color_ffxt color) {
    if (color & COLOR_HSV) { color = _fromHSV(color); }
    uint32_t opacity = (_getO(color) * 255) >> 4;
    return (opacity << 24) | (color & 0x00ffffff);
}

static int32_t lerp(int32_t a, int32_t b, fixed_ffxt t) {
    return scalarfx(a, FM_1 - t) + scalarfx(b, t);
}

color_ffxt ffx_color_lerpfx(color_ffxt c0, color_ffxt c1, fixed_ffxt t) {
    int32_t a = lerp(_getO(c0), _getO(c1), t);

    // Only differ in opacity
    if ((c0 & ~MASK_ALPHA) == (c1 & ~MASK_ALPHA)) {
        return (c0 & ~MASK_ALPHA) | ((MAX_OPACITY - a) << 24);
    }

    if ((c0 & COLOR_HSV) == 0) { c0 = _fromRGB(c0); }
    if ((c1 & COLOR_HSV) == 0) { c1 = _fromRGB(c1); }

    int32_t h = lerp(_getH(c0), _getH(c1), t);
    int32_t s = lerp(_getS(c0), _getS(c1), t);
    int32_t v = lerp(_getV(c0), _getV(c1), t);

    return ffx_color_hsva(h, s, v, a);
}

static int32_t lerpRatio(int32_t a, int32_t b, int32_t top, int32_t bottom) {
    return ((top * b) / bottom) + (((bottom - top) * a) / bottom);
}

color_ffxt ffx_color_lerpRatio(color_ffxt c0, color_ffxt c1,
  int32_t top, int32_t bottom) {

    int32_t a = lerpRatio(_getO(c0), _getO(c1), top, bottom);

    if ((c0 & COLOR_HSV) && (c1 & COLOR_HSV)) {
        int32_t h = lerpRatio(_getH(c0), _getH(c1), top, bottom);
        int32_t s = lerpRatio(_getS(c0), _getS(c1), top, bottom);
        int32_t v = lerpRatio(_getV(c0), _getV(c1), top, bottom);

        return ffx_color_hsva(h, s, v, a);
    }

    c0 = ffx_color_rgb24(c0);
    c1 = ffx_color_rgb24(c1);

    int32_t r = lerpRatio(_getR(c0), _getR(c1), top, bottom);
    int32_t g = lerpRatio(_getG(c0), _getG(c1), top, bottom);
    int32_t b = lerpRatio(_getB(c0), _getB(c1), top, bottom);

    return ffx_color_rgba(r, g, b, a);
}

color_ffxt ffx_color_lerpColorRamp(color_ffxt *colors, size_t count,
  fixed_ffxt t) {

    int32_t index = scalarfx(count - 1, t);
    if (index < 0) {
        return colors[0];
    } else if (index >= count - 1) {
        return colors[count - 1];
    }

    color_ffxt c0 = colors[index];
    color_ffxt c1 = colors[index + 1];

    return ffx_color_lerpfx(c0, c1, (t * (count - 1)) - tofx(index));
}
/*
rgba16_t ffx_color_blend16(color_t fg, color_t bg) {
    uint32_t fga = ALPHA(fg);
    if (fga == 0) { return ffx_color_rgb16(bg); }
    if (fga == MAX_ALPHA) { return ffx_color_rgb(fg);
    return ffx_color_lerpfx(bg, fg, tofx(fga) / MAX_ALPHA);
}
*/

color_ffxt ffx_color_rgb2hsv(color_ffxt color) {
    if (!(color & COLOR_HSV)) { return _fromRGB(color); }
    return color;
}

color_ffxt ffx_color_hsv2rgb(color_ffxt color) {
    if (color & COLOR_HSV) { return _fromHSV(color); }
    return color;
}

char* ffx_color_sprintf(color_ffxt c, char *name) {
    uint32_t opacity = _getO(c);

    if (opacity == 0) {
        snprintf(name, COLOR_STRING_LENGTH, "transparent");

    } else if (c & COLOR_HSV) {
        snprintf(name, COLOR_STRING_LENGTH,
          "HSV(%ld, %ld/63, %ld/63, %ld/32)", _getH(c), _getS(c), _getV(c),
          opacity);

    } else {
      snprintf(name, COLOR_STRING_LENGTH,
        "RGBA(%ld/255, %ld/255, %ld/255, %ld/32)", _getR(c), _getG(c),
        _getB(c), opacity);
    }

    return name;
}

FfxColorHSV ffx_color_parseHSV(color_ffxt color) {
    if (!(color & COLOR_HSV)) { color = _fromRGB(color); }

    FfxColorHSV hsv;
    hsv.hue = _getH(color);
    hsv.saturation = _getS(color);
    hsv.value = _getV(color);
    hsv.opacity = _getO(color);
    return hsv;
}

FfxColorRGB ffx_color_parseRGB(color_ffxt color) {
    if (color & COLOR_HSV) { color = _fromHSV(color); }

    FfxColorRGB rgb;
    rgb.red = _getR(color);
    rgb.blue = _getB(color);
    rgb.green = _getG(color);
    rgb.opacity = _getO(color);
    return rgb;
}

uint8_t ffx_color_getOpacity(color_ffxt color) {
    return _getO(color);
}

color_ffxt ffx_color_setOpacity(color_ffxt color, uint8_t opacity) {
    return (color & ~MASK_ALPHA) |
      ((MAX_OPACITY - CLAMP(opacity, MAX_OPACITY)) << 24);
}

bool ffx_color_isTransparent(color_ffxt color) {
    return (MASK_ALPHA & color) == 0x20000000;
}

//color_ffxt ffx_color_blend(color_ffxt foreground, color_ffxt background) {
//}

// Test program to validate HSV conversion
/*
int main() {
    for (uint32_t h  = 0; h < 360; h++) {
        for (uint32_t s  = 0; s <= 0x3f; s++) {
            for (uint32_t v = 0; v <= 0x3f; v++) {
                color_t hsv = color_hsv(h, s, v, 0xff);
                color_t rgb = _fromHSV(hsv);
                printf("h=%d, s=%d, v=%d, hsv=%u, rgb=%u\n", h,s, v, hsv, rgb);
            }
        }
    }
}
*/
