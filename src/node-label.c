#include <stddef.h>
#include <string.h>
#include <stdio.h>

#include "firefly-scene-private.h"
#include "firefly-color.h"

#include "fonts.h"


typedef struct LabelNode {
    FfxFont font;
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

FfxFontMetrics ffx_sceneLabel_getFontMetrics(FfxFont font) {
    FontInfo fontInfo = getFontInfo(font);

    const uint32_t header = fontInfo.font[0];

    return (FfxFontMetrics){
        .dimensions = (FfxSize){
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
// Methods

static void destroyFunc(FfxNode node) {
    ffx_sceneLabel_setText(node, NULL);
}

static void sequenceFunc(FfxNode node, FfxPoint worldPos) {
    LabelNode *label = ffx_sceneNode_getState(node);

    if (label->text == NULL) { return; }

    size_t strLen = strlen(label->text);
    if (strLen == 0) { return; }

    // Include the null termination and round up to the nearest word
    strLen = (strLen + 1 + 3) & 0xfffffc;

    FfxPoint pos = ffx_sceneNode_getPosition(node);
    pos.x += worldPos.x;
    pos.y += worldPos.y;

    LabelRender *render = ffx_scene_createRender(node, sizeof(LabelRender) + strLen);
    render->font = label->font;
    render->textColor = label->textColor;
    render->outlineColor = label->outlineColor;
    render->position = pos;

    strcpy((char*)&render[1], label->text);
}

static void renderGlyph(uint16_t *frameBuffer, int ox, int oy, int width,
  int height, const uint32_t *data, color_ffxt _color) {

    // Glyph is entirely outside the fragment; skip
    if (ox < -width || ox > 240 || oy < -height || oy > 24) {
        return;
    }
    // @TODO: add support for alpha blending colors. Note for outline
    //        alpha additional composite memory is required.

    uint16_t color = ffx_color_rgb16(_color);

    int x = 0, y = 0;
    // @TODO: Use pointer math instead of multiply
    //uint16_t *output = &frameBuffer[oy * 240 + ox]
    while (true) {
        uint32_t bitmap = *data++;
        for (int i = 0; i < 32; i++) {
            int tx = ox + x, ty = oy + y;
            if (bitmap & (0x80000000 >> i)) {
                if (tx >= 0 && tx < 240 && ty >= 0 && ty < 24) {
                    frameBuffer[ty * 240 + tx] = color;
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

#define SPACE_WIDTH       (2)
#define OUTLINE_WIDTH     (4)

static void renderText(uint16_t *frameBuffer, const char *text,
  FfxPoint position, const uint32_t *font, int32_t strokeOffset,
  color_ffxt color) {

    if (ffx_color_isTransparent(color)) { return; }

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

        renderGlyph(frameBuffer, x + gpl, y + gpt, gw, gh, data, color);
        x += width + SPACE_WIDTH;
    }
}

static void renderFunc(void *_render, uint16_t *frameBuffer,
  FfxPoint origin, FfxSize size) {

    LabelRender *render = _render;
    const char *text = (char*)&render[1];

    size_t length = strlen(text);

    FontInfo fontInfo = getFontInfo(render->font);
    const uint32_t *font = fontInfo.font;
    const uint32_t *outlineFont = fontInfo.outlineFont;

    //int32_t descent = (font[0] >> 16) & 0xff;
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

    //printf("TEXT CLIP: pos=%dx%d vp=%dx%d size=%dx%d\n", clip.x, clip.y,
    //  clip.vpX, clip.vpY, clip.width, clip.height);

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

    LabelNode *label = ffx_sceneNode_getState(node);

    char textColorName[COLOR_NAME_LENGTH] = { 0 };
    ffx_color_name(label->textColor, textColorName, sizeof(textColorName));

    char outlineColorName[COLOR_NAME_LENGTH] = { 0 };
    ffx_color_name(label->outlineColor, outlineColorName,
      sizeof(outlineColorName));

    int fontSize = label->font & FfxFontSizeMask;

    for (int i = 0; i < indent; i++) { printf("  "); }
    printf("<Label pos=%dx%d font=%dpt%s color=%s outline=%s text=\"%s\">\n", pos.x,
      pos.y, fontSize, (label->font & FfxFontBoldMask) ? "-bold": "",
      textColorName, outlineColorName, label->text);
}

static const FfxNodeVTable vtable = {
    .destroyFunc = destroyFunc,
    .sequenceFunc = sequenceFunc,
    .renderFunc = renderFunc,
    .dumpFunc = dumpFunc,
};


//////////////////////////
// Life-cycle

FfxNode ffx_scene_createLabel(FfxScene scene, FfxFont font, const char* text) {

    FfxNode node = ffx_scene_createNode(scene, &vtable, sizeof(LabelNode));

    LabelNode *label = ffx_sceneNode_getState(node);
    label->textColor = ffx_color_rgb(255, 255, 255, 0x3c);
    label->outlineColor = ffx_color_rgb(0, 0, 0, 0);
    label->font = font;

    ffx_sceneLabel_setText(node, text);

    return node;
}


//////////////////////////
// Properties

size_t ffx_sceneLabel_copyText(FfxNode node, char* output, size_t length) {
    LabelNode *label = ffx_sceneNode_getState(node);

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
    LabelNode *label = ffx_sceneNode_getState(node);

    if (label->text) {
        ffx_sceneNode_memFree(node, label->text);
        label->text = NULL;
    }

    size_t length = strlen(text);
    if (length) {
        label->text = ffx_sceneNode_memAlloc(node, length + 1);
        strcpy(label->text, text);
    } else {
        label->text = NULL;
    }
}

FfxFont ffx_sceneLabel_getFont(FfxNode node) {
    LabelNode *label = ffx_sceneNode_getState(node);
    return label->font;
}

void ffx_sceneLabel_setFont(FfxNode node, FfxFont font) {
    LabelNode *label = ffx_sceneNode_getState(node);
    label->font = font;
}

color_ffxt ffx_sceneLabel_getTextColor(FfxNode node) {
    LabelNode *label = ffx_sceneNode_getState(node);
    return label->textColor;
}

static void setTextColor(FfxNode node, color_ffxt color) {
    LabelNode *label = ffx_sceneNode_getState(node);
    label->textColor = color;
}

void ffx_sceneLabel_setTextColor(FfxNode node, color_ffxt color) {
    LabelNode *label = ffx_sceneNode_getState(node);
    ffx_sceneNode_createColorAction(node, label->textColor, color,
      setTextColor);
}

color_ffxt ffx_sceneLabel_getOutlineColor(FfxNode node) {
    LabelNode *label = ffx_sceneNode_getState(node);
    return label->outlineColor;
}

static void setOutlineColor(FfxNode node, color_ffxt color) {
    LabelNode *label = ffx_sceneNode_getState(node);
    label->outlineColor = color;
}

void ffx_sceneLabel_setOutlineColor(FfxNode node, color_ffxt color) {
    LabelNode *label = ffx_sceneNode_getState(node);
    ffx_sceneNode_createColorAction(node, label->outlineColor, color,
      setOutlineColor);
}


