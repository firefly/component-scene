import fs from "fs";

import { dirname, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';

import { Font } from "./bdf.js";

const __dirname = dirname(fileURLToPath(import.meta.url));
const base = resolve(__dirname, "../../fonts");

function loadFontData(filename: string): string{
    return fs.readFileSync(resolve(base, filename)).toString();
}

export function resolveFontPath(path: string): string {
    return resolve(base, path);
}

export type FontSize = "SMALL" | "MEDIUM" | "LARGE";
export type FontWeight = "NORMAL" | "BOLD";

export const FontSizes: ReadonlyArray<FontSize> = Object.freeze([ "SMALL", "MEDIUM", "LARGE" ]);
export const FontWeights: ReadonlyArray<FontWeight> = Object.freeze([ "NORMAL", "BOLD" ]);

const SizeMap:Record<FontSize, number> = {
  SMALL: 15, MEDIUM: 20, LARGE: 24 };

const Fonts: Array<{ filename: string, size: number, weight: FontWeight}> = [
    {
        filename: "neep-alt-iso8859-1-08x15.bdf",
        size: 15,
        weight: "NORMAL"
    },
    {
        filename: "neep-alt-iso8859-1-08x15-bold.bdf",
        size: 15,
        weight: "BOLD"
    },
    {
        filename: "neep-alt-iso8859-1-10x20.bdf",
        size: 20,
        weight: "NORMAL"
    },
    {
        filename: "neep-alt-iso8859-1-10x20-bold.bdf",
        size: 20,
        weight: "BOLD"
    },
    {
        filename: "neep-alt-iso8859-1-12x24.bdf",
        size: 24,
        weight: "NORMAL"
    },
    {
        filename: "neep-alt-iso8859-1-12x24-bold.bdf",
        size: 24,
        weight: "BOLD"
    },
];

export function loadFont(_size: FontSize, _weight: FontWeight): Font {
    for (const { filename, size, weight } of Fonts) {
        if (SizeMap[_size] === size && _weight === weight) {
            return Font.fromBdf(loadFontData(filename), filename);
        }
    }
    throw new Error(`unknown font: size=${ _size } weight=${ _weight }`);
}


//const dot = Bitmap.createDot(2);
//console.log(dot);
//dot.dump();
/*
const font = loadFont("LARGE", "NORMAL");

for (let i = 33; i < 127; i++) {
  let bitmap = font.getBitmap(String.fromCharCode(i));
  bitmap.dump();
  bitmap.trimmed().dump();

  bitmap = bitmap.outlined(3).trimmed();
  bitmap.dump();
}
*/
