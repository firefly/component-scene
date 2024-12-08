// See: https://adobe-type-tools.github.io/font-tech-notes/pdfs/5005.BDF_Spec.pdf
var __classPrivateFieldSet = (this && this.__classPrivateFieldSet) || function (receiver, state, value, kind, f) {
    if (kind === "m") throw new TypeError("Private method is not writable");
    if (kind === "a" && !f) throw new TypeError("Private accessor was defined without a setter");
    if (typeof state === "function" ? receiver !== state || !f : !state.has(receiver)) throw new TypeError("Cannot write private member to an object whose class did not declare it");
    return (kind === "a" ? f.call(receiver, value) : f ? f.value = value : state.set(receiver, value)), value;
};
var __classPrivateFieldGet = (this && this.__classPrivateFieldGet) || function (receiver, state, kind, f) {
    if (kind === "a" && !f) throw new TypeError("Private accessor was defined without a getter");
    if (typeof state === "function" ? receiver !== state || !f : !state.has(receiver)) throw new TypeError("Cannot read private member from an object whose class did not declare it");
    return kind === "m" ? f : kind === "a" ? f.call(receiver) : f ? f.value : state.get(receiver);
};
var _Bitmap_data, _Font_data;
const skip = "COMMENT ENDCHAR ENDFONT ENDPROPERTIES".split(" ");
export class Box {
    constructor(width, height, x, y) {
        this.x = x;
        this.y = y;
        this.width = width;
        this.height = height;
    }
    toString() {
        return `<Bounds x=${this.x} y=${this.y} width=${this.width} height=${this.height}`;
    }
    static fromBbx(bbx) {
        const values = bbx.split(/\s+/).map(x => (parseInt(x.trim())));
        // @TODO: chck 4 numbes
        return new Box(...values);
    }
}
function emptyScanline(width) {
    const result = [];
    while (result.length < width) {
        result.push(0);
    }
    return result;
}
export class Bitmap {
    constructor(data) {
        _Bitmap_data.set(this, void 0);
        if (data.length === 0) {
            throw new Error("no data");
        }
        data.reduce((a, r) => {
            if (a === -1) {
                return r.length;
            }
            if (a !== r.length) {
                throw new Error("invalid row length");
            }
            return a;
        }, -1);
        __classPrivateFieldSet(this, _Bitmap_data, data, "f");
        this.width = data[0].length;
        this.height = data.length;
    }
    getBit(x, y) {
        const value = __classPrivateFieldGet(this, _Bitmap_data, "f")[y][x];
        if (value == null) {
            throw new Error(`out of range: ${x}x${y}`);
        }
        return !!value;
    }
    forEach(callback) {
        __classPrivateFieldGet(this, _Bitmap_data, "f").forEach((row, y) => {
            row.forEach((bit, x) => {
                if (bit) {
                    callback(x, y, this);
                }
            });
        });
    }
    get data() {
        return __classPrivateFieldGet(this, _Bitmap_data, "f").map((r) => r.slice());
    }
    trim() {
        let data = __classPrivateFieldGet(this, _Bitmap_data, "f").slice();
        while (data.length) {
            const sum = data[0].reduce((a, v) => { return a + v; }, 0);
            if (sum) {
                break;
            }
            data.shift();
        }
        while (data.length) {
            const sum = data[data.length - 1].reduce((a, v) => { return a + v; }, 0);
            if (sum) {
                break;
            }
            data.pop();
        }
        while (data[0].length) {
            const sum = data.reduce((a, v) => { return a + v[0]; }, 0);
            if (sum) {
                break;
            }
            data.forEach((v) => { v.shift(); });
        }
        while (data[0].length) {
            const sum = data.reduce((a, v) => { return a + v[v.length - 1]; }, 0);
            if (sum) {
                break;
            }
            data.forEach((v) => { v.pop(); });
        }
        return new Bitmap(data);
    }
    static fromData(data, bounds) {
        const bbx = Box.fromBbx(data.BBX);
        if (!bounds) {
            bounds = bbx;
        }
        const b = bounds;
        const bitmap = [];
        for (let y = 0; y < bounds.height; y++) {
            bitmap.push(emptyScanline(bounds.width));
        }
        const setBit = (x, y) => {
            x -= b.x;
            y -= b.y;
            y = (b.height - 1) - y;
            if (bitmap[y] == null || bitmap[y][x] == null) {
                throw new Error(`out of bounds: bounds=${b.toString()}, x=${x}, y=${y}`);
                //console.log("WARNING: drawing out of bounds");
                //return;
            }
            bitmap[y][x] = 1;
        };
        const scanlines = data.bitmap.split(" ").reverse();
        let y = bbx.y;
        for (const _line of scanlines) {
            let x = bbx.x;
            const line = parseInt(_line, 16);
            let mask = 1 << ((_line.length * 4) - 1);
            for (let i = 0; i < bbx.width; i++) {
                if (line & mask) {
                    setBit(x, y);
                }
                mask >>= 1;
                x++;
            }
            y++;
        }
        return new Bitmap(bitmap);
    }
}
_Bitmap_data = new WeakMap();
export class Font {
    constructor(data, filename) {
        _Font_data.set(this, void 0);
        __classPrivateFieldSet(this, _Font_data, data, "f");
        this.bounds = Box.fromBbx(this.getData("__").FONTBOUNDINGBOX);
        this.filename = filename || "<unknown>";
        const metadata = data.get("__") || {};
        let name = metadata.FAMILY_NAME || metadata.FONT || "<unknown>";
        if (name[0] === '"' && name[name.length - 1] === '"') {
            name = name.substring(1, name.length - 1);
        }
        this.name = name;
    }
    getData(char) {
        const data = __classPrivateFieldGet(this, _Font_data, "f").get(char);
        if (data == null) {
            throw new Error(`no glyph data found: ${JSON.stringify(char)}`);
        }
        return data;
    }
    getBitmap(char) {
        return Bitmap.fromData(this.getData(char), this.bounds);
    }
    static fromBdf(content, filename) {
        let data = new Map();
        // Accumulate the header metadata into "__"
        let current = {};
        data.set("__", current);
        for (let line of content.split("\n")) {
            line = line.trim();
            const space = line.indexOf(" ");
            if (space === -1) {
                if (skip.indexOf(line) >= 0 || line === "") {
                    // pass
                }
                else if (line === "BITMAP") {
                    // Initialize a field for the bitmap
                    current.bitmap = "";
                }
                else if (line.match(/^[0-9A-F]+$/i) && "bitmap" in current) {
                    // Reading a bitmap entry
                    current.bitmap += (current.bitmap ? " " : "") + line;
                }
                else {
                    throw new Error(`unhandled line: ${JSON.stringify(line)}`);
                }
                continue;
            }
            // The key-value pair of the line
            const key = line.substring(0, space).trim();
            const value = line.substring(space).trim();
            // Start a new character
            if (key === "STARTCHAR") {
                current = {};
            }
            // Found the character codepoint; set it in the mapping
            if (key === "ENCODING") {
                data.set(String.fromCharCode(parseInt(value)), current);
            }
            // Add the field to the character
            current[key] = value;
        }
        return new Font(data, filename);
    }
}
_Font_data = new WeakMap();
//# sourceMappingURL=bdf.js.map