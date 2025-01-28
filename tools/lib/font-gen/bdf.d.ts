export declare class Box {
    readonly x: number;
    readonly y: number;
    readonly width: number;
    readonly height: number;
    constructor(width: number, height: number, x: number, y: number);
    toString(): string;
    static fromBbx(bbx: string): Box;
}
export declare class Bitmap {
    #private;
    readonly width: number;
    readonly height: number;
    readonly padLeft: number;
    readonly padTop: number;
    constructor(data: Array<Array<0 | 1>>, padLeft: number, padTop: number);
    getBit(x: number, y: number): boolean;
    setBit(x: number, y: number, on: boolean): void;
    forEach(callback: (x: number, y: number, isOn: boolean) => void, mask?: boolean): void;
    get data(): Array<Array<0 | 1>>;
    trimmed(): Bitmap;
    expanded(count: number): Bitmap;
    dump(): void;
    clone(): Bitmap;
    paste(bitmap: Bitmap, x: number, y: number, mask?: boolean): void;
    outlined(radius: number): Bitmap;
    static createDot(radius: number): Bitmap;
    static create(width: number, height: number): Bitmap;
    static fromData(data: Record<string, string>, bounds?: Box): Bitmap;
}
export declare class Font {
    #private;
    readonly bounds: Box;
    readonly filename: string;
    readonly name: string;
    constructor(data: Map<string, Record<string, string>>, filename?: string);
    getData(char: string): Record<string, string>;
    getBitmap(char: string): Bitmap;
    static fromBdf(content: string, filename?: string): Font;
}
//# sourceMappingURL=bdf.d.ts.map