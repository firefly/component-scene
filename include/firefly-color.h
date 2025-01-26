#ifndef __FIREFLY_COLOR_H__
#define __FIREFLY_COLOR_H__

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#include <stddef.h>
#include <stdint.h>

#include "firefly-curves.h"


typedef struct FfxColorRGB {
    int32_t red;
    int32_t blue;
    int32_t green;
    int32_t opacity;
} FfxColorRGB;

typedef struct FfxColorHSV {
    int32_t hue;
    int32_t saturation;
    int32_t value;
    int32_t opacity;
} FfxColorHSV;

// RGB565 Format
typedef uint16_t rgb16_ffxt;

// 0x00RRGGBB
typedef uint32_t rgb24_ffxt;

// 0xAARRGGBB
typedef uint32_t rgba24_ffxt;


// General color format that can be used to indicate a generic
// color in either RGBA and HSVA color spaces. Values are
// clamped.
// - RGBA: 8-bits per color; opacity=[0,32]
// - HSVA: 12-bits for hue,; 6-bits for saturation, value; opacity=[0,32]
typedef uint32_t color_ffxt;


///////////////////////////////
// Colors

// Stores RGB565 as a uint16_t
#define RGB16(r,g,b)       ((((uint32_t)(r) & 0xf8) << 8) | (((uint32_t)(g) & 0xfc) << 3) | (((uint32_t)(b) & 0xf8) >> 3))

#define RGBA_DARKER75       (0x18000000)
#define RGBA_DARKER50       (0x10000000)
#define RGBA_DARKER25       (0x08000000)

#define MAX_OPACITY          (0x20)

#define OPACITY_100          (32)
#define OPACITY_90           (29)
#define OPACITY_80           (26)
#define OPACITY_70           (22)
#define OPACITY_60           (19)
#define OPACITY_50           (16)
#define OPACITY_40           (13)
#define OPACITY_30           (10)
#define OPACITY_20            (6)
#define OPACITY_10            (3)
#define OPACITY_0             (0)

///////////////////////////////
// Creating color

/**
 *  Convert an RGBA tuple into a color. All values are clamped to
 *  their respective depths; see above.
 */
color_ffxt ffx_color_rgb(int32_t r, int32_t g, int32_t b, int32_t opacity);

/**
 *  Convert an HSVA tuple into a color. All values are clamped to
 *  their respective depths; see above.
 *
 *  If the hue is outside the range [0, 3959], it is moved to
 *  an "earlier" rotation within the range.
 */
color_ffxt ffx_color_hsv(int32_t hue, int32_t sat, int32_t val,
  int32_t opacity);


///////////////////////////////
// Conversion

/**
 *  Convert a color from the RGB color space to the HSV color space.
 */
color_ffxt ffx_color_rgb2hsv(color_ffxt color);

/**
 *  Convert a color from the HSV color space to the RGB color space.
 */
color_ffxt ffx_color_hsv2rgb(color_ffxt color);

/**
 *  Return the R5G6B5 value that most closely matches %%color%%.
 */
rgb16_ffxt ffx_color_rgb16(color_ffxt color);

/**
 *  Return the RGB888 value of %%color%%.
 *
 * If color is HSV, it is converted. Any opacity is dropped.
 */
rgb24_ffxt ffx_color_rgb24(color_ffxt color);

/**
 *  Return the ARGB8888 value of %%color%%.
 *
 * If color is HSV, it is converted. The opacity is scaled to 8-bit.
 */
rgba24_ffxt ffx_color_rgba24(color_ffxt color);


///////////////////////////////
// Inspection

#define COLOR_NAME_LENGTH     (40)

/**
 *  Describes %%color%% into %%name%% in upto %%length%% bytes.
 *
 *  e.g.
 *    - "RGB(128/255, 0/255, 12/255, 16/32)"
 *    - "HSV(275, 63/63, 59/63, 16/32)"
 */
size_t ffx_color_name(color_ffxt color, char *name, size_t length);

FfxColorHSV ffx_color_parseHSV(color_ffxt color);
FfxColorRGB ffx_color_parseRGB(color_ffxt color);

uint8_t ffx_color_getOpacity(color_ffxt color);

bool ffx_color_isTransparent(color_ffxt color);


///////////////////////////////
// Interpolation

// Linear-interpolate from color a to b at parametric value t. When
// the color-spaces (RGB vs HSV) differ, values are coerced to RGBA.
color_ffxt ffx_color_lerpfx(color_ffxt c0, color_ffxt c1, fixed_ffxt t);
color_ffxt ffx_color_lerpRatio(color_ffxt c0, color_ffxt c1, int32_t top, int32_t bottom);

/**
 *  Linearly interpolate across the %%colors%% at %%t%%.
 */
color_ffxt ffx_color_lerpColorRamp(color_ffxt *colors, size_t count,
  fixed_ffxt t);

color_ffxt ffx_color_blend(color_ffxt foreground, color_ffxt background);

//color_t color_lerp(color_t a, color_t b, uint32_t top, uint32_t bottom);

//rgb16_t color_rgb16Lerp(color_t a, color_t b, uint32_t top, uint32_t bottom);
//rgba16_t color_rgba16Lerp(color_t a, color_t b, uint32_t top, uint32_t bottom);
//rgb16_t color_rgb16Lerpfx(color_t a, color_t b, fixed_t alpha);
//rgba16_t color_rgba16Lerpfx(color_t a, color_t b, fixed_t alpha);

//rgb24_t color_rgb24Lerp(color_t a, color_t b, uint32_t top, uint32_t bottom);
//rgb24_t color_rgb24Lerpfx(color_t a, color_t b, fixed_t alpha);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __FIREFLY_COLOR_H__ */
