import { Font } from "./bdf.js";
export declare function resolveFontPath(path: string): string;
export type FontSize = "SMALL" | "MEDIUM" | "LARGE";
export type FontWeight = "NORMAL" | "BOLD";
export declare const FontSizes: ReadonlyArray<FontSize>;
export declare const FontWeights: ReadonlyArray<FontWeight>;
export declare function loadFont(_size: FontSize, _weight: FontWeight): Font;
//# sourceMappingURL=fonts.d.ts.map