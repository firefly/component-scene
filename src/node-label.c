#include <stdarg.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>

#include "firefly-scene-private.h"
#include "firefly-color.h"

#include "fonts.h"


typedef struct LabelNode {
    FfxFont font;
    FfxTextAlign align;
    color_ffxt textColor;
    color_ffxt outlineColor;
    char *text;
} LabelNode;

typedef struct LabelRender {
    FfxPoint position;
    FfxFont font;
    color_ffxt textColor;
    color_ffxt outlineColor;
    // Text goes here
} LabelRender;


static bool walkFunc(FfxNode node, FfxNodeVisitFunc enterFunc,
  FfxNodeVisitFunc exitFunc, void* arg);
static void destroyFunc(FfxNode node);
static void sequenceFunc(FfxNode node, FfxPoint worldPos);
static void renderFunc(void *_render, uint16_t *_frameBuffer,
  FfxPoint origin, FfxSize size);
static void dumpFunc(FfxNode node, int indent);

static const char name[] = "LabelNode";
static const FfxNodeVTable vtable = {
    .walkFunc = walkFunc,
    .destroyFunc = destroyFunc,
    .sequenceFunc = sequenceFunc,
    .renderFunc = renderFunc,
    .dumpFunc = dumpFunc,
    .name = name
};


#define SPACE_WIDTH       (2)
#define OUTLINE_WIDTH     (4)


//////////////////////////
// Utilities

typedef struct FontInfo {
    const uint32_t *font;
    const uint32_t *outlineFont;
    uint32_t outlineWidth;
} FontInfo;


FontInfo getFontInfo(FfxFont font) {
    FontInfo result = { };
    result.outlineWidth = 2;  // @TODO: this changed

    switch (font) {

        case FfxFontSmall:
            result.font = font_small_normal;
            result.outlineFont = font_small_normal_outline;
            result.outlineWidth = 3;
            break;
        case FfxFontSmallBold:
            result.font = font_small_bold;
            result.outlineFont = font_small_bold_outline;
            break;

        case FfxFontMedium:
            result.font = font_medium_normal;
            result.outlineFont = font_medium_normal_outline;
            break;
        case FfxFontMediumBold:
            result.font = font_medium_bold;
            result.outlineFont = font_medium_bold_outline;
            break;

        case FfxFontLarge:
            result.font = font_large_normal;
            result.outlineFont = font_large_normal_outline;
            break;
        case FfxFontLargeBold:
            result.font = font_large_bold;
            result.outlineFont = font_large_bold_outline;
            break;

        default:
            printf("unknown font: %d", font);
    }

    return result;
}

FfxFontMetrics ffx_scene_getFontMetrics(FfxFont font) {
    FontInfo fontInfo = getFontInfo(font);

    const uint32_t header = fontInfo.font[0];

    return (FfxFontMetrics){
        .size = (FfxSize){
            .width = (header >> 0) & 0xff,
            .height = (header >> 8) & 0xff
        },
        .descent = (header >> 16) & 0xff,
        .outlineWidth = fontInfo.outlineWidth,
        .points = (font & FfxFontSizeMask),
        .isBold = !!(font & FfxFontBoldMask)
    };
}


//////////////////////////
// Rasterizing

// NOTE: Alpha blending outlineColor is not currently supported as it
//       requires memory allocated for a composite layer.

static void renderGlyphOpaque(uint16_t *frameBuffer, int ox, int oy, int width,
  int height, const uint32_t *data, color_ffxt _color) {

    // Glyph is entirely outside the fragment; skip
    if (ox < -width || ox > 240 || oy < -height || oy > 24) { return; }

    // Get the color, broken into its pre-multiplied components
    uint16_t fg = ffx_color_rgb16(_color);

    int x = 0, y = 0;
    // @TODO: Use pointer math instead of multiply
    //uint16_t *output = &frameBuffer[oy * 240 + ox]
    while (true) {
        uint32_t bitmap = *data++;
        for (int i = 0; i < 32; i++) {
            int tx = ox + x, ty = oy + y;
            if (bitmap & (0x80000000 >> i)) {
                if (tx >= 0 && tx < 240 && ty >= 0 && ty < 24) {
                    frameBuffer[ty * 240 + tx] = fg;
                }
            }

            x++;
            if (x >= width) {
                x = 0;
                y++;
                if (y >= height) { return; }
            }
        }
    }
}

static void renderGlyphBlend(uint16_t *frameBuffer, int ox, int oy, int width,
  int height, const uint32_t *data, color_ffxt _color) {

    // Glyph is entirely outside the fragment; skip
    if (ox < -width || ox > 240 || oy < -height || oy > 24) { return; }

    // Get the alpha and alpha inverse (ufixed:1.16)
    uint32_t fga = FIXED_BITS_5(ffx_color_getOpacity(_color));
    uint32_t fga_1 = FM_1 - fga;

    // Get the color, broken into its pre-multiplied components
    uint16_t fg = ffx_color_rgb16(_color);
    int fgpmR = fg >> 11;
    fgpmR *= fga;
    int fgpmG = (fg >> 5) & 0x3f;
    fgpmG *= fga;
    int fgpmB = fg & 0x1f;;
    fgpmB *= fga;

    int x = 0, y = 0;
    // @TODO: Use pointer math instead of multiply
    //uint16_t *output = &frameBuffer[oy * 240 + ox]
    while (true) {
        uint32_t bitmap = *data++;
        for (int i = 0; i < 32; i++) {
            int tx = ox + x, ty = oy + y;
            if (bitmap & (0x80000000 >> i)) {
                if (tx >= 0 && tx < 240 && ty >= 0 && ty < 24) {

                    if (fga >= FM_1) {
                        // 100% opaque
                        frameBuffer[ty * 240 + tx] = fg;

                    } else {

                        // Get the current color...
                        uint16_t bg = frameBuffer[ty * 240 + tx];
                        int bgR = bg >> 11;
                        int bgG = (bg >> 5) & 0x3f;
                        int bgB = bg & 0x1f;

                        // Blend the values and convert from fixed-point
                        int blendR = (fgpmR + (fga_1 * bgR)) >> 16;
                        int blendG = (fgpmG + (fga_1 * bgG)) >> 16;
                        int blendB = (fgpmB + (fga_1 * bgB)) >> 16;

                        frameBuffer[ty * 240 + tx] = (blendR << 11) |
                          (blendG << 5) | blendB;
                    }
                }
            }

            x++;
            if (x >= width) {
                x = 0;
                y++;
                if (y >= height) { return; }
            }
        }
    }
}

static void renderText(uint16_t *frameBuffer, const char *text,
  FfxPoint position, const uint32_t *font, int32_t strokeOffset,
  color_ffxt color) {

    uint8_t opacity = ffx_color_getOpacity(color);

    if (opacity == 0) { return; }

    int32_t width = (font[0] >> 0) & 0xff;

    int x = position.x - strokeOffset, y = position.y - strokeOffset;

    int i = 0;
    while (true) {
        char c = text[i++];

        // NULL-termination
        if (c == 0) { break; }

        // non-printable character; @TODO: add placeholder
        if (c <= ' ' || c > '~') {
            x += width + SPACE_WIDTH;
            continue;
        }

        int index = (c - ' ');
        int gw = (font[index] >> 27) & 0x1f;
        int gh = (font[index] >> 22) & 0x1f;
        int gpl = ((font[index] >> 18) & 0x0f) - 6;
        int gpt = ((font[index] >> 13) & 0x1f) - 6;
        int offset = (font[index] & 0x1fff);
        const uint32_t *data = &font[95 + offset];

        if (opacity == MAX_OPACITY) {
            renderGlyphOpaque(frameBuffer, x + gpl, y + gpt, gw, gh, data, color);
        } else {
            renderGlyphBlend(frameBuffer, x + gpl, y + gpt, gw, gh, data, color);
        }
        x += width + SPACE_WIDTH;
    }
}


//////////////////////////
// Methods

static bool walkFunc(FfxNode node, FfxNodeVisitFunc enterFunc,
  FfxNodeVisitFunc exitFunc, void* arg) {

    if (enterFunc && !enterFunc(node, arg)) { return false; }
    if (exitFunc && !exitFunc(node, arg)) { return false; }
    return true;
}

static void destroyFunc(FfxNode node) {
    ffx_sceneLabel_setText(node, NULL);
}

static const FfxTextAlign MASK_VERTICAL = FfxTextAlignMiddle |
  FfxTextAlignBottom | FfxTextAlignMiddleBaseline | FfxTextAlignBaseline |
  FfxTextAlignTop;

static const FfxTextAlign MASK_HORIZONTAL = FfxTextAlignCenter |
  FfxTextAlignRight | FfxTextAlignLeft;

static void sequenceFunc(FfxNode node, FfxPoint worldPos) {

    FfxPoint pos = ffx_sceneNode_getPosition(node);
    pos.x += worldPos.x;
    pos.y += worldPos.y;

    LabelNode *label = ffx_sceneNode_getState(node, &vtable);
    if (label->text == NULL) { return; }

    FfxFontMetrics metrics = ffx_scene_getFontMetrics(label->font);

    // Handle vertical alignment
    switch (label->align & MASK_VERTICAL) {
        case FfxTextAlignMiddle:
            pos.y -= metrics.size.height / 2;
            break;
        case FfxTextAlignBottom:
            pos.y -= metrics.size.height;
            break;
        case FfxTextAlignMiddleBaseline:
            pos.y -= (metrics.size.height / 2) - metrics.descent;
            break;
        case FfxTextAlignBaseline:
            pos.y -= metrics.size.height - metrics.descent;
            break;

        case FfxTextAlignTop: default:
            break;
    }

    if (pos.y >= 240 || pos.y + metrics.size.height < 0) { return; }

    size_t strLen = strlen(label->text);
    if (strLen == 0) { return; }

    int width = (metrics.size.width + SPACE_WIDTH) * strLen - 2;
    switch(label->align & MASK_HORIZONTAL){
        case FfxTextAlignCenter:
            pos.x -= width / 2;
            break;
        case FfxTextAlignRight:
            pos.x -= width;
            break;
        case FfxTextAlignLeft: default:
            break;
    }

    if (pos.x > 240 || pos.x + width <= 0) { return; }

    LabelRender *render = ffx_scene_createRender(node, sizeof(LabelRender) +
      ((strLen + 1 + 3) & 0xfffffc)); // @TODO: move this to alloc?
    render->font = label->font;
    render->textColor = label->textColor;
    render->outlineColor = label->outlineColor;
    render->position = pos;

    strcpy((char*)&render[1], label->text);
}


static void renderFunc(void *_render, uint16_t *frameBuffer,
  FfxPoint origin, FfxSize size) {

    LabelRender *render = _render;
    const char *text = (char*)&render[1];

    size_t length = strlen(text);

    FontInfo fontInfo = getFontInfo(render->font);
    const uint32_t *font = fontInfo.font;
    const uint32_t *outlineFont = fontInfo.outlineFont;

    int32_t height = (font[0] >> 8) & 0xff;
    int32_t width = (font[0] >> 0) & 0xff;

    FfxPoint pos = (FfxPoint){ .x = -OUTLINE_WIDTH, .y = -OUTLINE_WIDTH };
    pos.x += render->position.x;
    pos.y += render->position.y;

    FfxClip clip = ffx_scene_clip(render->position, (FfxSize){
        .width = (2 * OUTLINE_WIDTH) + ((length - 1) * (width + SPACE_WIDTH)),
        .height = (2 * OUTLINE_WIDTH) + height
    }, origin, size);

    if (clip.width == 0) { return; }

    FfxPoint position = {
        .x = render->position.x,
        .y = render->position.y - origin.y
    };

    renderText(frameBuffer, text, position, outlineFont, 0,
      render->outlineColor);
    renderText(frameBuffer, text, position, font, 0,
      render->textColor);
}

static void dumpFunc(FfxNode node, int indent) {
    FfxPoint pos = ffx_sceneNode_getPosition(node);

    LabelNode *label = ffx_sceneNode_getState(node, &vtable);

    char textColorName[COLOR_STRING_LENGTH] = { 0 };
    ffx_color_sprintf(label->textColor, textColorName);

    char outlineColorName[COLOR_STRING_LENGTH] = { 0 };
    ffx_color_sprintf(label->outlineColor, outlineColorName);

    int fontSize = label->font & FfxFontSizeMask;

    for (int i = 0; i < indent; i++) { printf("  "); }
    printf("<Label pos=%dx%d font=%dpt%s color=%s outline=%s text=\"%s\">\n",
      pos.x, pos.y, fontSize,
      (label->font & FfxFontBoldMask) ? "-bold": "", textColorName,
      outlineColorName, label->text);
}


//////////////////////////
// Life-cycle

FfxNode ffx_scene_createLabel(FfxScene scene, FfxFont font, const char* text) {

    FfxNode node = ffx_scene_createNode(scene, &vtable, sizeof(LabelNode));

    LabelNode *label = ffx_sceneNode_getState(node, &vtable);
    label->textColor = ffx_color_rgb(255, 255, 255);
    label->outlineColor = ffx_color_rgba(0, 0, 0, 0);
    label->font = font;

    ffx_sceneLabel_setText(node, text);

    return node;
}

bool ffx_scene_isLabel(FfxNode node) {
    return ffx_scene_isNode(node, &vtable);
}


//////////////////////////
// Properties

size_t ffx_sceneLabel_getTextLength(FfxNode node) {
    LabelNode *label = ffx_sceneNode_getState(node, &vtable);
    if (label == NULL) { return 0; }

    if (label->text == NULL) { return 0; }

    return strlen(label->text);
}

size_t ffx_sceneLabel_copyText(FfxNode node, char* output, size_t length) {
    LabelNode *label = ffx_sceneNode_getState(node, &vtable);
    if (label == NULL) { return 0; }

    if (length == 0) { return 0; }

    if (label->text == NULL) {
        output[0] = 0;
        return 0;
    }

    char c = 1;
    int i = 0;
    while (c && i < length) {
        c = label->text[i];
        output[i++] = c;
    }

    return i - 1;
}

void ffx_sceneLabel_setText(FfxNode node, const char* text) {
    LabelNode *label = ffx_sceneNode_getState(node, &vtable);
    if (label == NULL) { return; }

    if (label->text) {
        ffx_sceneNode_memFree(node, label->text);
        label->text = NULL;
    }

    if (text == NULL) { return; }

    size_t length = strlen(text);
    if (length) {
        label->text = ffx_sceneNode_memAlloc(node, length + 1);
        strcpy(label->text, text);
    } else {
        label->text = NULL;
    }
}

void ffx_sceneLabel_setTextFormat(FfxNode node, const char* format, ...) {
    char *str = NULL;

    va_list args;
    va_start(args, format);
    int length = vasprintf(&str, format, args);
    va_end(args);

    if (length == -1 || str == NULL) {
        printf("ERROR\n");
        return;
    }

    ffx_sceneLabel_setText(node, str);

    free(str);
}

void ffx_sceneLabel_appendText(FfxNode node, const char* text) {
    size_t curLength = ffx_sceneLabel_getTextLength(node);
    char curText[curLength + strlen(text) + 1];
    ffx_sceneLabel_copyText(node, curText, curLength + 1);
    strcpy((char*)&curText[curLength], text);
    ffx_sceneLabel_setText(node, curText);
}

void ffx_sceneLabel_appendCharacter(FfxNode node, char chr) {
    char text[] = { chr, '\0' };
    ffx_sceneLabel_appendText(node, text);
}

void ffx_sceneLabel_appendFormat(FfxNode node, const char* format, ...) {
    char *str = NULL;

    va_list args;
    va_start(args, format);
    int length = vasprintf(&str, format, args);
    va_end(args);

    if (length == -1 || str == NULL) {
        printf("ERROR\n");
        return;
    }

    ffx_sceneLabel_appendText(node, str);

    free(str);
}

void ffx_sceneLabel_insertText(FfxNode node, size_t offset, const char* text) {

    size_t curLen = ffx_sceneLabel_getTextLength(node);
    if (curLen >= offset) {
        ffx_sceneLabel_appendText(node, text);
        return;
    }

    size_t len = strlen(text);

    char curText[curLen + len + 1];
    ffx_sceneLabel_copyText(node, curText, curLen + 1);

    // Make room for the inserted text
    int i = curLen + len + 1;
    while (i >= offset + len) { // ?? >? >=?
        curText[i] = curText[i - len];
        i--;
    }

    // Insert the text
    for (int i = 0; i < len; i++) { curText[offset + i] = text[i]; }

    ffx_sceneLabel_setText(node, text);
}

void ffx_sceneLabel_insertCharacter(FfxNode node, size_t offset, char chr) {
    char text[] = { chr, '\0' };
    ffx_sceneLabel_insertText(node, offset, text);
}

void ffx_sceneLabel_insertFormat(FfxNode node, size_t offset,
  const char* format, ...) {

    char *str = NULL;

    va_list args;
    va_start(args, format);
    int length = vasprintf(&str, format, args);
    va_end(args);

    if (length == -1 || str == NULL) {
        printf("ERROR\n");
        return;
    }

    ffx_sceneLabel_insertText(node, offset, str);

    free(str);
}

void ffx_sceneLabel_snipText(FfxNode node, size_t offset, size_t length) {
    size_t curLen = ffx_sceneLabel_getTextLength(node);

    if (offset > curLen) { offset = curLen; }
    if (offset + length > curLen) { length = curLen - offset; }

    if (length == 0) { return; }

    // Get the current text
    char curText[curLen + 1];
    ffx_sceneLabel_copyText(node, curText, sizeof(curText));

    for (int i = offset; i <= curLen - length; i++) {
        curText[i] = curText[i + length];
    }

    ffx_sceneLabel_setText(node, curText);
}

FfxTextAlign ffx_sceneLabel_getAlign(FfxNode node) {
    LabelNode *label = ffx_sceneNode_getState(node, &vtable);
    if (label == NULL) { return 0; }
    return label->align;
}

void ffx_sceneLabel_setAlign(FfxNode node, FfxTextAlign align) {
    LabelNode *label = ffx_sceneNode_getState(node, &vtable);
    if (label == NULL) { return; }
    label->align = align;
}

FfxFont ffx_sceneLabel_getFont(FfxNode node) {
    LabelNode *label = ffx_sceneNode_getState(node, &vtable);
    if (label == NULL) { return 0; }
    return label->font;
}

void ffx_sceneLabel_setFont(FfxNode node, FfxFont font) {
    LabelNode *label = ffx_sceneNode_getState(node, &vtable);
    if (label == NULL) { return; }
    label->font = font;
}

color_ffxt ffx_sceneLabel_getTextColor(FfxNode node) {
    LabelNode *label = ffx_sceneNode_getState(node, &vtable);
    if (label == NULL) { return 0; }
    return label->textColor;
}

static void setTextColor(FfxNode node, color_ffxt color) {
    LabelNode *label = ffx_sceneNode_getState(node, &vtable);
    if (label == NULL) { return; }
    label->textColor = color;
}

void ffx_sceneLabel_setTextColor(FfxNode node, color_ffxt color) {
    LabelNode *label = ffx_sceneNode_getState(node, &vtable);
    if (label == NULL) { return; }
    if (label->textColor == color) { return; }
    ffx_sceneNode_createColorAction(node, label->textColor, color,
      setTextColor);
}

void ffx_sceneLabel_setOpacity(FfxNode node, uint8_t opacity) {
    LabelNode *label = ffx_sceneNode_getState(node, &vtable);
    if (label == NULL) { return; }
    ffx_sceneLabel_setTextColor(node, ffx_color_setOpacity(label->textColor,
      opacity));
}

static void animateTextColor(FfxNode node, FfxNodeAnimation *animation,
  void *arg) {
    color_ffxt *color = arg;
    ffx_sceneLabel_setTextColor(node, *color);
}

void ffx_sceneLabel_animateTextColor(FfxNode node, color_ffxt color,
  uint32_t delay, uint32_t duration, FfxCurveFunc curve,
  FfxNodeAnimationCompletionFunc onComplete, void* arg) {

    LabelNode *label = ffx_sceneNode_getState(node, &vtable);
    if (label == NULL) { return; }
    if (label->textColor == color) { return; }

    ffx_sceneNode_runAnimation(node, animateTextColor, &color, delay, duration,
      curve, onComplete, arg);
}

void ffx_sceneLabel_animateOpacity(FfxNode node, uint8_t opacity,
  uint32_t delay, uint32_t duration, FfxCurveFunc curve,
  FfxNodeAnimationCompletionFunc onComplete, void* arg) {

    LabelNode *label = ffx_sceneNode_getState(node, &vtable);
    if (label == NULL) { return; }
    color_ffxt color = ffx_color_setOpacity(label->textColor, opacity);

    ffx_sceneLabel_animateTextColor(node, color, delay, duration, curve,
      onComplete, arg);
}

color_ffxt ffx_sceneLabel_getOutlineColor(FfxNode node) {
    LabelNode *label = ffx_sceneNode_getState(node, &vtable);
    if (label == NULL) { return 0; }
    return label->outlineColor;
}

static void setOutlineColor(FfxNode node, color_ffxt color) {
    LabelNode *label = ffx_sceneNode_getState(node, &vtable);
    if (label == NULL) { return; }
    label->outlineColor = color;
}

void ffx_sceneLabel_setOutlineColor(FfxNode node, color_ffxt color) {
    LabelNode *label = ffx_sceneNode_getState(node, &vtable);
    if (label == NULL) { return; }
    if (label->outlineColor == color) { return; }
    ffx_sceneNode_createColorAction(node, label->outlineColor, color,
      setOutlineColor);
}


