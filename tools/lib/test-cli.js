import fs from "fs";
import Jimp from "jimp";
import { ImageRGB, ImageRGBA } from "./image-rgb.js";
import { toDotH } from "./dot-h.js";
(async function () {
    let format = "RGB";
    let filename = "";
    let tag = "tag";
    let i = 2;
    while (i < process.argv.length) {
        const arg = process.argv[i++];
        if (arg.startsWith("--")) {
            if (arg === "--tag") {
                tag = process.argv[i++];
            }
            else if (arg === "--rgb") {
                format = "RGB";
            }
            else if (arg === "--rgba") {
                format = "RGBA";
            }
            else {
                throw new Error(`unknown flag: ${arg}`);
            }
            continue;
        }
        if (filename == "") {
            filename = arg;
        }
        else {
            throw new Error("already has filename");
        }
    }
    const data = fs.readFileSync(filename);
    const jimp = await Jimp.read(data);
    let image;
    switch (format) {
        case "RGB":
            image = ImageRGB.fromImage(jimp);
            break;
        case "RGBA":
            image = ImageRGBA.fromImage(jimp);
            break;
        default:
            throw new Error(`unknown format: ${format}`);
    }
    const output = toDotH(image.bytes, tag);
    console.log(output);
})();
//# sourceMappingURL=test-cli.js.map