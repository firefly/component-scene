import { GifUtil } from "gifwrap";
import { Jimp } from "jimp";

import type { JimpInstance } from "jimp";

export interface Frame {
    image: JimpInstance;
    duration: number;
}

export async function readGif(data: Uint8Array): Promise<Array<Frame>> {
    const gif = await GifUtil.read(Buffer.from(data));

    const result: Array<Frame> = [ ];
    const lastFrame = new Jimp({
        width: gif.width,
        height: gif.height,
        color: 0
    });

    for (const frame of gif.frames) {
        const { data, width, height } = frame.bitmap;
        const img = new Jimp({ width, height, color: 0 });
        img.bitmap.data.set(data);

        lastFrame.composite(img, frame.xOffset, frame.yOffset);;

        result.push({
            image: lastFrame.clone(),
            duration: (frame.delayCentisecs * 10)
        });
    }

    return result;
}
