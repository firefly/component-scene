
import { Jimp } from "jimp";

import { loadFont, resolveFontPath, FontSizes, FontWeights } from "./fonts.js";


import type { JimpInstance } from "jimp";
import type { Font } from "./bdf.js";



const BLACK = "#000000", WHITE = "#ffffff";
const text0 = "The quick brown fox jumps over the lazy dog.";

const padding = [ 0, 25, 0, 25 ];

function shl(v: number, shift: number): number {
    return Number(BigInt(v) << BigInt(shift));
}


export async function createFontPreview(font: Font, text: string, isDark: boolean) {
    const { width, height } = font.bounds;
    const c = isDark ? 0xffffff: 0;

    const preview = new Jimp({
        width: (width + 1) * 48 + padding[1] + padding[3],
        height: 8 * (height + 1),
        color: (isDark ? BLACK: WHITE)
    });
    const setPixel = (x: number, y: number) => {
        const offset = y * preview.bitmap.width + (padding[1] + x);
        preview.bitmap.data[4 * offset] = (c >> 16) & 0xff;
        preview.bitmap.data[4 * offset + 1] = (c >> 8) & 0xff;
        preview.bitmap.data[4 * offset + 2] = (c >> 0) & 0xff;
    };

    const putChar = (chr: string, bx: number, by: number) => {
        const bitmap = font.getBitmap(chr);
        bitmap.forEach((x, y) => { setPixel(bx + x, by + y); });
        //setPixel(bx, by);
        //setPixel(bx + width - 1, by + height - 1);
    };

    for (let i = 0; i < text.length; i++) {
        putChar(text[i], i * (width + 1), 1 * (height + 1));
    }

    for (let i = 33; i < 127; i++) {
        const chr = String.fromCharCode(i);
        const index = i - 33;
        const bx = (index % 48) * (width + 1);
        const by = (3 + Math.trunc(index / 48)) * (height + 1);

        putChar(chr, bx, by);
    }

    for (let i = 0; i < text0.length; i++) {
        putChar(text0[i], i * (width + 1), 6 * (height + 1));
    }

    return preview;
}

async function createPreview(isDark: boolean) {
    const previews: Array<JimpInstance> = [ ];

    for (const size of FontSizes) {
        for (const weight of FontWeights) {
            const font = loadFont(size, weight);
            const text = `font-${ size.toLowerCase() }-${ weight.toLowerCase() }`;
            previews.push(await createFontPreview(font, text, isDark));
        }
    }

    {
        const maxWidth = previews.reduce((a, img) => {
            return Math.max(img.bitmap.width, a);
        }, 0);
        const sumHeight = previews.reduce((a, img) => {
            return img.bitmap.height + a;
        }, 0);

        const preview = new Jimp({
            width: maxWidth,
            height: sumHeight,
            color: (isDark ? BLACK: WHITE)
        });
        let y = 0;
        for(const img of previews) {
            preview.composite(img, 0, y);
            y += img.bitmap.height;
        }
        return preview
    }

}

(async function() {
    for (const isDark of [ false, true ]) {
        const preview = await createPreview(isDark);
        const path = resolveFontPath(`preview-${ isDark ? "dark": "light" }.png`);
        preview.write(<any>path); // THIS IS DUMB; can it be fixed? Jimp enforces an extension in TS-world
    }
})();

function getValue(v: string): number {
    if (v.length > 32) { throw new Error("Hmm"); }
    while (v.length < 32) { v += "0"; }
    return parseInt(v, 2);
}

(async function() {

    const doth: Array<string> = [ ];

    let totalSize = 0;

    const toHex = (_v: number) => {
        let v = _v.toString(16)
        while (v.length < 8) { v = "0" + v; }
        return `0x${ v }`;
    }

    const addData = (header: string, data: Array<number>, footer?: string) => {
        doth.push("");
        doth.push(`  /\/ ${ header }`);
        for (let i = 0; i < data.length; i += 6) {
            doth.push("  " + data.slice(i, i + 6).map(toHex).join(", ") + ",");
        }
        if (footer) {
          doth.push(`  /\/ ${ footer }`);
        }
    }

    let minPadTop = 10000, maxPadTop = -10000;
    let minPadLeft = 10000, maxPadLeft = -10000;

    const generateFont = (font: Font, outline: number) => {

        if (font.bounds.x !== 0 || font.bounds.y > 0 || -font.bounds.y > 31) {
            throw new Error(`unsupported font bounds: ${ font.bounds.toString() }`);
        }

        //console.log();
        //console.log({ width: font.bounds.width, height: font.bounds.height,
        //  descent: -font.bounds.y, outline, font: font.filename });


        const indices: Array<number> = [ ];
        const widths: Array<number> = [ ];
        const heights: Array<number> = [ ];
        const padLefts: Array<number> = [ ];
        const padTops: Array<number> = [ ];
        const data: Array<number> = [ ];

        for (let i = 33; i < 127; i++) {
            // Compute the glyph bitmap
            let bitmap = font.getBitmap(String.fromCharCode(i));
            if (outline) {
                bitmap = bitmap.outlined(outline);
            }
            bitmap = bitmap.trimmed();

            // Debug
            //if (i === 65) { bitmap.dump(); }

            // Compute the index data
            if (data.length > 8191) {
                throw new Error(`unsupported index: ${ data.length }`);
            }
            indices.push(data.length);

            // Compute the bitmap size data
            if (bitmap.width > 31 || bitmap.height > 31) {
                throw new Error(`unsuppoers glyph size: width=${ bitmap.width } height=${ bitmap.height }`);
            }
            widths.push(bitmap.width);
            heights.push(bitmap.height);

            // Compute the bitmap padding data
            if (bitmap.padLeft < -6 || bitmap.padLeft > 9 ||
              bitmap.padTop < -6 || bitmap.padTop > 25) {
                throw new Error(`unsupported pad size: left=${ bitmap.padLeft } top=${ bitmap.padTop }`);
            }
            padLefts.push(bitmap.padLeft);
            padTops.push(bitmap.padTop);
            if (bitmap.padLeft < minPadLeft) { minPadLeft = bitmap.padLeft; }
            if (bitmap.padLeft > maxPadLeft) { maxPadLeft = bitmap.padLeft; }
            if (bitmap.padTop < minPadTop) { minPadTop = bitmap.padTop; }
            if (bitmap.padTop > maxPadTop) { maxPadTop = bitmap.padTop; }

            // Compute bitmap data
            let v = '';
            for (let y = 0; y < bitmap.height; y++) {
                for (let x = 0; x < bitmap.width; x++) {
                    if (v.length === 32) {
                        data.push(getValue(v));
                        v = '';
                    }
                    v += bitmap.getBit(x, y) ? "1": "0";
                }
            }
            if (v.length > 0) { data.push(getValue(v)); }
        }

        //console.log({ indices, widths, heights, padLefts, padTops, data });

        addData(`Font Info: width=${ font.bounds.width } height=${ font.bounds.height } descent=${ -font.bounds.y }`, [
            shl((-font.bounds.y), 16) +
            shl(font.bounds.height, 8) +
            shl(font.bounds.width, 0)
        ]);
        totalSize += 1;

        // [ width:5 ] [ height:5 ] [ padLeft:4 ] [ padTop:5 ] [ index:13 ]
        addData(`Glyph Info:`, indices.map((index, i) => {
            return shl(widths[i], 27) +
                shl(heights[i], 22) +
                shl((padLefts[i] + 6), 18) +
                shl((padTops[i] + 6), 13) +
                shl(index, 0)
        }));
        totalSize += indices.length;

        addData("Bitmap Data:", data, `Font Bytes: ${ data.length * 4 }`);
        totalSize += data.length;
    };

    doth.push(`#ifndef __FONTS_H__`);
    doth.push(`#define __FONTS_H__`);
    doth.push("");
    doth.push("#ifdef __cplusplus");
    doth.push("extern \"C\" {");
    doth.push("#endif  /* __cplusplus */");
    doth.push("#include <stdint.h>");

    for (const size of FontSizes) {
        for (const weight of FontWeights) {
            for (let outline of [ 0, 1 ]) {
                const font = loadFont(size, weight);
                if (outline) {
                    if (weight == "BOLD") {
                        outline = 4;
                    } else {
                        outline = 3;
                    }
                }

                doth.push("");
                doth.push(`/\/ Font: ${ size.toLowerCase() }-${ weight.toLowerCase() }${ outline ? "-outline": ""} (${ font.filename })`);
                doth.push(`const uint32_t font_${ size.toLowerCase() }_${ weight.toLowerCase() }${ outline ? "_outline": "" }[] = {`);

                generateFont(font, outline);

                doth.push("");
                doth.push(`  ${ toHex(0) }`);
                totalSize += 1;

                doth.push("};");
            }
        }
    }

    doth.push("");
    doth.push(`/\/ Total Size: ${ totalSize * 4 }`);

    doth.push("");
    doth.push("#ifdef __cplusplus");
    doth.push("}");
    doth.push("#endif  /* __cplusplus */");
    doth.push("");
    doth.push(`#endif  /* __FONTS_H__ */`);

    console.log(doth.join("\n"));
    //console.log("//", { minPadTop, minPadLeft, maxPadTop, maxPadLeft });
})();
