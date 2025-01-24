
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

typedef struct FontInfo {
    const uint32_t *font;
    const uint32_t *outlineFont;
    uint32_t outlineWidth;
} FontInfo;

FontInfo getFontInfo(FfxFont font) {
    FontInfo result = { };
    result.outlineWidth = 2;

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

static void destroyFunc(FfxNode node) {
    LabelNode *state = ffx_sceneNode_getState(node);
    if (state->text) {
        ffx_sceneNode_memFree(node, state->text);
        state->text = NULL;
    }
}

static void sequenceFunc(FfxNode node, FfxPoint worldPos) {
    LabelNode *state = ffx_sceneNode_getState(node);

    FfxPoint pos = ffx_sceneNode_getPosition(node);
    pos.x += worldPos.x;
    pos.y += worldPos.y;

    // Round up to the nearest word
    size_t strLen = (strlen(state->text) + 1 + 3) & 0xfffffc;

    LabelRender *render = ffx_scene_createRender(node, sizeof(LabelRender) + strLen);
    render->font = state->font;
    render->textColor = state->textColor;
    render->outlineColor = state->outlineColor;
    render->position = pos;

    strcpy((char*)&render[1], state->text);
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

static void renderText(uint16_t *frameBuffer, const char *text,
  FfxPoint position, const uint32_t *font, int32_t strokeOffset,
  color_ffxt color) {

    if (ffx_color_isTransparent(color)) { return; }

    //int32_t descent = (font[0] >> 16) & 0xff;
    //int32_t height = (font[0] >> 8) & 0xff;
    int32_t width = (font[0] >> 0) & 0xff;

    int x = position.x - strokeOffset, y = position.y - strokeOffset;

    int i = 0;
    while (true) {
        char c = text[i++];

        // NULL-termination
        if (c == 0) { break; }

        // non-printable character; @TODO: add placeholder
        if (c <= ' ' || c > '~') {
            x += width + 1;
            continue;
        }

        int index = (c - ' ');
        int gw = (font[index] >> 27) & 0x1f;
        int gh = (font[index] >> 22) & 0x1f;
        int gpl = ((font[index] >> 16) & 0x3f) - 31;
        int gpt = ((font[index] >> 10) & 0x3f) - 31;
        int offset = font[index] & 0x3ff;
        const uint32_t *data = &font[95 + offset];

        renderGlyph(frameBuffer, x + gpl, y + gpt, gw, gh, data, color);
        x += width + 1;
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

    // @TODO: Account for outline in this clipping

    FfxClip clip = ffx_scene_clip(render->position, (FfxSize){
        .width = length * (width + 1) - 1, .height = height
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

    LabelNode *state = ffx_sceneNode_getState(node);

    char textColorName[COLOR_NAME_LENGTH] = { 0 };
    ffx_color_name(state->textColor, textColorName, sizeof(textColorName));

    char outlineColorName[COLOR_NAME_LENGTH] = { 0 };
    ffx_color_name(state->outlineColor, outlineColorName,
      sizeof(outlineColorName));

    for (int i = 0; i < indent; i++) { printf("  "); }
    printf("<Label pos=%dx%d font=%d color=%s outline=%s text=\"%s\">\n", pos.x,
      pos.y, state->font, textColorName, outlineColorName, state->text);
}

static const _FfxNodeVTable vtable = {
    .destroyFunc = destroyFunc,
    .sequenceFunc = sequenceFunc,
    .renderFunc = renderFunc,
    .dumpFunc = dumpFunc,
};

FfxNode ffx_scene_createLabel(FfxScene scene, FfxFont font, const char* text) {

    FfxNode node = ffx_scene_createNode(scene, &vtable, sizeof(LabelNode));

    LabelNode *state = ffx_sceneNode_getState(node);
    state->textColor = ffx_color_rgb(255, 255, 255, 0x3c);
    state->outlineColor = ffx_color_rgb(0, 0, 0, 0);
    state->font = font;

    ffx_sceneLabel_setText(node, text);

    return node;
}

size_t ffx_sceneLabel_copyText(FfxNode node, char* output, size_t length) {
    LabelNode *state = ffx_sceneNode_getState(node);

    if (length == 0) { return 0; }

    if (state->text == NULL) {
        output[0] = 0;
        return 0;
    }

    char c = 1;
    int i = 0;
    while (c && i < length) {
        c = state->text[i];
        output[i++] = c;
    }

    return i - 1;
}

void ffx_sceneLabel_setText(FfxNode node, const char* text) {
    LabelNode *state = ffx_sceneNode_getState(node);

    if (state->text) {
        ffx_sceneNode_memFree(node, state->text);
        state->text = NULL;
    }

    size_t length = strlen(text);
    state->text = ffx_sceneNode_memAlloc(node, length + 1);
    strcpy(state->text, text);
}

FfxFont ffx_sceneLabel_getFont(FfxNode node) {
    LabelNode *state = ffx_sceneNode_getState(node);
    return state->font;
}

void ffx_sceneLabel_setFont(FfxNode node, FfxFont font) {
    LabelNode *state = ffx_sceneNode_getState(node);
    state->font = font;
}

color_ffxt ffx_sceneLabel_getTextColor(FfxNode node) {
    LabelNode *state = ffx_sceneNode_getState(node);
    return state->textColor;
}

void ffx_sceneLabel_setTextColor(FfxNode node, color_ffxt color) {
    LabelNode *state = ffx_sceneNode_getState(node);
    state->textColor = color;
}

color_ffxt ffx_sceneLabel_getOutlineColor(FfxNode node) {
    LabelNode *state = ffx_sceneNode_getState(node);
    return state->outlineColor;
}

void ffx_sceneLabel_setOutlineColor(FfxNode node, color_ffxt color) {
    LabelNode *state = ffx_sceneNode_getState(node);
    state->outlineColor = color;
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
