// LibreSprite
// Copyright (C) 2024 LibreSprite contributors

var dialog, intSeed;

const tweakRain = false;
const MaxSize = 175;
let lastResult;

const defSettings = {
    FAt : 0.1,
    CAt : 0.2,
    DAt : 0.1997,
    speedAt : 0.8,
    LT : 50,
    FR : 0.8
}

let FAt = defSettings.FAt;
let CAt = defSettings.CAt;
let speedAt = defSettings.speedAt;
let DAt = defSettings.DAt;
let LT = defSettings.LT;
let FR = defSettings.FR;
var Blend = 75;

function noise(X, Y, W, H, seed) {
    seed = seed|0;
    X = (X ^ seed) * W | 0;
    Y = (Y ^ seed) * H | 0;
    const data = new Float32Array(W * H);
    for (let y = 0; y < H; ++y) {
        for (let x = 0; x < W; ++x) {
            data[y * W + x] = rng(x, y);
        }
    }
    return data;
    function rng(x, y) {
        return (((((X + x) * 1991 ^ (Y + y) * 2731) * seed) >>> 0) & 0xFFFF) / 0xFFFF;
    }
}

function cloud(X, Y, W, H, seed) {
    seed = seed|0;
    let data = noise(X, Y, W, H, seed);
    X = (X ^ seed) * W | 0;
    Y = (Y ^ seed) * H | 0;
    for (let l = 1; l < 8; ++l) {
        let L = 1 << l;
        for (let y = 0; y < H; y += L) {
            for (let x = 0; x < W; x += L) {
                let A = rng(x, y) * L;
                let B = rng(x + L, y) * L;
                let C = rng(x, y + L) * L;
                let D = rng(x + L, y + L) * L;
                for (let iy = 0; iy < L; ++iy) {
                    let w = iy / L;
                    let AC = A * (1 - w) + C * w;
                    let BD = B * (1 - w) + D * w;
                    for (let ix = 0; ix < L; ++ix) {
                        w = ix / L;
                        let ACBD = AC * (1 - w) + BD * w;
                        data[(y + iy) * W + (x + ix)] += ACBD;
                    }
                }

            }
        }
    }

    for (let l = 0; l < W*H; ++l)
        data[l] /= 255;

    return data;

    function rng(x, y) {
        return (((((X + x) * 1991 ^ (Y + y) * 2731) * seed) >>> 0) & 0xFFFF) / 0xFFFF;
    }
}

function rain(heightmap, W, H, seed, cycles) {
    seed = seed|0;
    const X = seed * W | 0;
    const Y = seed * H | 0;
    const data = new Float32Array(heightmap);
    blur(data, W, H);

    for (let ix = 0; ix < cycles; ++ix) {
        let y = Math.random() * H;
        let x = Math.random() * W;
        let vx = FAt * (Math.random() * 2 - 1);
        let vy = FAt * (Math.random() * 2 - 1);

        for (let i = 0; i < LT; ++i) {
            let py = y|0;
            let px = x|0;

            let s = Math.sqrt(vx*vx + vy*vy);
            if (s) {
                y += vy/s;
                x += vx/s;
                let w = i/LT;
                data[py * W + px] -= s * w * CAt;
                data[(y|0) * W + (x|0)] += s * w * DAt;
            }
            vx *= FR;
            vy *= FR;

            let A = data[(y  |0) * W + (x  |0)];
            if (A === undefined) break;

            const r = 1;
            let B = data[((y-r)|0) * W + ((x  )|0)] ?? A;
            let C = data[((y  )|0) * W + ((x+r)|0)] ?? A;
            let D = data[((y+r)|0) * W + ((x  )|0)] ?? A;
            let E = data[((y  )|0) * W + ((x-r)|0)] ?? A;
            let F = data[((y-r)|0) * W + ((x-r)|0)] ?? A;
            let G = data[((y-r)|0) * W + ((x+r)|0)] ?? A;
            let H = data[((y+r)|0) * W + ((x+r)|0)] ?? A;
            let I = data[((y+r)|0) * W + ((x-r)|0)] ?? A;

            let AB = A - B;
            let AC = A - C;
            let AD = A - D;
            let AE = A - E;
            let AF = A - F;
            let AG = A - G;
            let AH = A - H;
            let AI = A - I;

            let X = AB;
            let dx = 0, dy = -1;
            if (AC > X) {
                dx = 1;
                dy = 0;
                X = AC;
            }
            if (AD > X) {
                dx = 0;
                dy = 1;
                X = AD;
            }
            if (AE > X) {
                dx = -1;
                dy = 0;
                X = AE;
            }
            if (AF > X) {
                dx = -1;
                dy = -1;
                X = AF;
            }
            if (AG > X) {
                dx = +1;
                dy = -1;
                X = AG;
            }
            if (AH > X) {
                dx = +1;
                dy = +1;
                X = AH;
            }
            if (AI > X) {
                dx = -1;
                dy = +1;
                X = AI;
            }

            vx += dx * X * speedAt;
            vy += dy * X * speedAt;
        }
    }

    for (let i = 0; i < data.length; ++i) {
        let v = data[i];        // 0 ... 1
        if (v < 0) v = 0;
        else if (v > 1) v = 1;
        data[i] = v;
    }
    blur(data, W, H);
    return data;
    function rng(x, y) {
        return (((((X + x) * 1991 ^ (Y + y) * 2731) * seed) >>> 0) & 0xFFFF) / 0xFFFF;
    }
}

function blur(data, W, H) {
    for (let y = 0; y < H - 1; ++y) {
        for (let x = 0; x < W - 1; ++x) {
            let i = y * W + x;
            let a = data[i];
            a += data[i + 1];
            a += data[i + W];
            a += data[i + W + 1];
            data[i] = a / 4;
        }
    }
    return data;
}

function sigma(data, p = 0) {
    if (p <= 0)
        return data;
    for (let i = 0; i < data.length; ++i) {
        let v = data[i];        // 0 ... 1
        if (v < 0) v = 0;
        else if (v > 1) v = 1;
        v *= 2;                 // 0 ... 2
        v--;                    // -1 ... 1
        if (v == 0)
            continue;
        if (v > 0) {            // 0 ... 1
            v = 1 - v;          // 1 ... 0
            v = v ** p;         // 1 ... 0
            v = 1 - v;          // 0 ... 1
        } else {                // -1 ... 0
            v = 1 + v;          // 0 ... 1
            v = v ** p;         // 0 ... 1
            v = v - 1;          // -1 ... 0
        }
        v++;                    // 0 ... 2
        v *= 0.5;               // 0 ... 1
        data[i] = v;
    }
    return data;
}

function vectorSigma(data, pVec) {
    for (let i = 0; i < data.length; ++i) {
        let p = pVec[i];
        if (p <= 0)
            continue;
        let v = data[i];        // 0 ... 1
        if (v < 0) v = 0;
        else if (v > 1) v = 1;
        v *= 2;                 // 0 ... 2
        v--;                    // -1 ... 1
        if (v == 0)
            continue;
        if (v > 0) {            // 0 ... 1
            v = 1 - v;          // 1 ... 0
            v = v ** p;         // 1 ... 0
            v = 1 - v;          // 0 ... 1
        } else {                // -1 ... 0
            v = 1 + v;          // 0 ... 1
            v = v ** p;         // 0 ... 1
            v = v - 1;          // -1 ... 0
        }
        v++;                    // 0 ... 2
        v *= 0.5;               // 0 ... 1
        data[i] = v;
    }
    return data;
}

function scale(data, scale) {
    for (let i = 0; i < data.length; ++i) {
        data[i] *= scale;
    }
    return data;
}

function vectorScale(data, scale) {
    for (let i = 0; i < data.length; ++i) {
        data[i] *= scale[i];
    }
    return data;
}

function vectorOverlay(data, light, blend = 0.5) {
    for (let i = 0; i < data.length; ++i) {
        let a = light[i];
        let b = data[i];
        let v;
        if (b < 0.5) {
            v = 2 * a * b;
        } else {
            v = 1 - 2 * (1 - a) * (1 - b);
        }
        data[i] = b * (1 - blend) + v * blend;
    }
    return data;
}

function vectorHardLight(data, light, blend = 0.5) {
    for (let i = 0; i < data.length; ++i) {
        let a = light[i];
        let b = data[i];
        let v;
        if (a < 0.5) {
            v = 2 * a * b;
        } else {
            v = 1 - 2 * (1 - a) * (1 - b);
        }
        data[i] = b * (1 - blend) + v * blend;
    }
    return data;
}

function vectorSoftLight(data, light, blend = 0.5) {
    for (let i = 0; i < data.length; ++i) {
        let a = light[i];
        let b = data[i];
        let v = (1 - 2*b) * (a*a) + 2 * b * a;
        data[i] = b * (1 - blend) + v * blend;
    }
    return data;
}

function terrain(X, Y, W, H, seed, cycles) {
    const heightmap = vectorSigma(
        cloud(X, Y, W, H, seed),
        scale(sigma(cloud(X, Y, W, H, seed + 85821), 3), 3)
    );
    const erosionmap = rain(heightmap, W, H, seed, cycles);
    vectorSoftLight(heightmap, erosionmap, Blend / 100);
    return heightmap;
}

const eventHandlers = {
    init:function(){
        if (!app.activeSprite) {
            app.createDialog('Error').addLabel('Need an image.');
            return;
        }

        if (dialog)
            dialog.close();
        dialog = app.createDialog('dlgHeightmap');
        intSeed = dialog.addEntry("Seed:", "intSeed");
        intSeed.value = "" + ((storage.get("intSeed") || Math.random() * 0xFFFF) | 0);
        dialog.addButton("Random", "btnRand");
        dialog.addBreak();
        dialog.addEntry("X:", "intX").value = "0";
        dialog.addEntry("Y:", "intY").value = "0";
        dialog.addBreak();
        dialog.addEntry("Erosion (0-9):", "intRain").value = storage.get("intRain") || "7";
        dialog.addEntry("Blending (0-100):", "intBlend").value = storage.get("intBlend") || "75";

        if (tweakRain) {
            dialog.addBreak();
            dialog.addEntry("C (" + defSettings.CAt + ")", "intC").value = storage.get("intC") || defSettings.CAt + "";
            dialog.addEntry("D (" + defSettings.DAt + ")", "intD").value = storage.get("intD") || defSettings.DAt + "";
            dialog.addBreak();
            dialog.addEntry("N (" + defSettings.FAt + ")", "intF").value = storage.get("intF") || defSettings.FAt + "";
            dialog.addEntry("S (" + defSettings.speedAt + ")", "intS").value = storage.get("intS") || defSettings.speedAt + "";
            dialog.addBreak();
            dialog.addEntry("L (" + defSettings.LT + ")", "intLT").value = storage.get("intLT") || defSettings.LT + "";
            dialog.addEntry("F (" + defSettings.FR + ")", "intFR").value = storage.get("intFR") || defSettings.FR + "";
        } else {
            storage.set(defSettings.FAt + "", "intF");
            storage.set(defSettings.CAt + "", "intC");
            storage.set(defSettings.speedAt + "", "intS");
            storage.set(defSettings.DAt + "", "intD");
            storage.set(defSettings.LT + "", "intLT");
            storage.set(defSettings.FR + "", "intFR");
        }
        dialog.addBreak();
        dialog.addButton("Generate", "run");
        dialog.addButton("Export STL", "export");
    },

    dlgHeightmap_close:function(){
        dialog = null;
        intSeed = null;
    },

    btnRand_click:function(){
        intSeed.value = "" + (Math.random() * 0xFFFF | 0);
        eventHandlers.run_click();
    },

    export_click:function(){
        if (!lastResult)
            eventHandlers.run_click();
        if (!lastResult)
            return;

        const fileName = `terrain_${lastResult.seed}`;
        const triangulator = getMarchingCubes();
        let {positions, cells} = triangulator([lastResult.W + 2, 258, lastResult.H + 2],
                                              sampler,
                                              [[0,0,0], [lastResult.W + 2, 1.2, lastResult.H + 2]]);
        const S = Math.min(MaxSize / (lastResult.W + 2), MaxSize / (lastResult.H + 2), MaxSize / 256);
        const scale = [S, MaxSize * S, S];

        storage.set(STLBIN(), 'stl', fileName);
        let path = storage.save('stl', fileName);

        function STLBIN() {
            const Float = new Float32Array(1);
            const Uint32 = new Uint32Array(Float.buffer);
            const Bytes = new Uint8Array(Float.buffer);

            const src = [];
            for (let i = 0; i < 80; ++i)
                src.push(0);

            Uint32[0] = cells.length;
            src.push(...Bytes);

            for (let index of cells) {
                Float[0] = 0;
                src.push(...Bytes, ...Bytes, ...Bytes);
                for (let i of index) {
                    for (let j of [2, 0, 1]) {
                        Float[0] = positions[i][j] * scale[j];
                        src.push(...Bytes);
                    }
                }
                src.push(0, 0);
            }
            return Uint8Array.from(src);
        }

        function STL() {
            let src = [];
            src.push(`solid heightmap`);
            for (let index of cells) {
                src.push(`facet normal 0 0 0`);
                src.push(`    outer loop`);
                for (let i of index) {
                    let [x, y, z] = positions[i];
                    src.push(`        vertex ${z * scale.z} ${x * scale.x} ${y * scale.y}`);
                }
                src.push(`    endloop`);
                src.push(`endfacet`);
            }
            src.push(`endsolid heightmap`);
            return src.join('\n');
        }

        app.createDialog('Error').addLabel(path ? `Exported ${path}` : `Could not save ${fileName}`);

        function sampler(x, y, z) {
            x = (x - 1) | 0;
            if (x < 0 || x >= lastResult.W) return 0;
            y = y - 0.1;
            if (y < 0) return 0;
            z = (z - 1) | 0;
            if (z < 0 || z >= lastResult.H) return 0;
            return lastResult.heightmap[z * lastResult.W + x] - y;
        }
    },

    Error_close:function(){},
    intSeed_change:function(){},
    intX_change:function(){},
    intY_change:function(){},
    intF_change:function(){},
    intC_change:function(){},
    intS_change:function(){},
    intD_change:function(){},
    intLT_change:function(){},
    intFR_change:function(){},
    intRain_change:function(){},
    intBlend_change:function(){},

    run_click:function(){
        let seed = storage.get("intSeed")|0;
        let firstSeed = seed;
        for (let i = 0; i < 10; ++i)
            seed = (seed + ((seed * 6763 + 7879) ^ (seed * 4391 + 9227))) >>> 0;

        const X = storage.get("intX")|0;
        const Y = storage.get("intY")|0;
        let Rain = (storage.get("intRain")|0);
        Blend = (storage.get("intBlend")|0);

        if (Rain > 9)
            Rain = 9;
        if (Blend > 100) Blend = 100;
        if (Blend < 0) Blend = 0;

        FAt = parseFloat(storage.get("intF"));
        CAt = parseFloat(storage.get("intC"));
        speedAt = parseFloat(storage.get("intS"));
        DAt = parseFloat(storage.get("intD"));
        LT = parseInt(storage.get("intLT"));
        FR = parseFloat(storage.get("intFR"));

        const sprite = app.activeSprite;
        const layerNumber = app.activeLayerNumber;
        const reference = sprite.layer(layerNumber).cel(0).image;
        const refWidth = reference.width;
        const refHeight = reference.height;
        const data = reference.getImageData();

        app.command.setParameter("content", "current");

        for (let frameNumber = 0; frameNumber < 1; ++frameNumber) {
            let frame = sprite.layer(layerNumber).cel(frameNumber);
            if (!frame) {
                app.command.NewFrame();
                frame = sprite.layer(layerNumber).cel(frameNumber);
            }

            const n = terrain(X, Y, refWidth, refHeight, seed + frameNumber, Rain ** 7);
            lastResult = {heightmap:n, W:refWidth, H:refHeight, seed:firstSeed};

            for (let y = 0; y < refHeight; ++y) {
                for (let x = 0; x < refWidth; ++x) {
                    let i = (y * refWidth + x) * 4;
                    let h = n[y * refWidth + x] * 255;
                    data[i++] = h;
                    data[i++] = h;
                    data[i++] = h;
                    data[i++] = 255;
                }
            }

            frame.image.putImageData(data);
        }
    }
};

function onEvent(eventName, ...args) {
    var handler = eventHandlers[eventName];
    if (typeof handler == 'function')
        handler();
}

function getMarchingCubes() {
    /**
     * Javascript Marching Cubes
     *
     * Based on Paul Bourke's classic implementation:
     *    http://local.wasp.uwa.edu.au/~pbourke/geometry/polygonise/
     *
     * JS port by Mikola Lysenko
     */

    var edgeTable= new Uint32Array([
        0x0  , 0x109, 0x203, 0x30a, 0x406, 0x50f, 0x605, 0x70c,
        0x80c, 0x905, 0xa0f, 0xb06, 0xc0a, 0xd03, 0xe09, 0xf00,
        0x190, 0x99 , 0x393, 0x29a, 0x596, 0x49f, 0x795, 0x69c,
        0x99c, 0x895, 0xb9f, 0xa96, 0xd9a, 0xc93, 0xf99, 0xe90,
        0x230, 0x339, 0x33 , 0x13a, 0x636, 0x73f, 0x435, 0x53c,
        0xa3c, 0xb35, 0x83f, 0x936, 0xe3a, 0xf33, 0xc39, 0xd30,
        0x3a0, 0x2a9, 0x1a3, 0xaa , 0x7a6, 0x6af, 0x5a5, 0x4ac,
        0xbac, 0xaa5, 0x9af, 0x8a6, 0xfaa, 0xea3, 0xda9, 0xca0,
        0x460, 0x569, 0x663, 0x76a, 0x66 , 0x16f, 0x265, 0x36c,
        0xc6c, 0xd65, 0xe6f, 0xf66, 0x86a, 0x963, 0xa69, 0xb60,
        0x5f0, 0x4f9, 0x7f3, 0x6fa, 0x1f6, 0xff , 0x3f5, 0x2fc,
        0xdfc, 0xcf5, 0xfff, 0xef6, 0x9fa, 0x8f3, 0xbf9, 0xaf0,
        0x650, 0x759, 0x453, 0x55a, 0x256, 0x35f, 0x55 , 0x15c,
        0xe5c, 0xf55, 0xc5f, 0xd56, 0xa5a, 0xb53, 0x859, 0x950,
        0x7c0, 0x6c9, 0x5c3, 0x4ca, 0x3c6, 0x2cf, 0x1c5, 0xcc ,
        0xfcc, 0xec5, 0xdcf, 0xcc6, 0xbca, 0xac3, 0x9c9, 0x8c0,
        0x8c0, 0x9c9, 0xac3, 0xbca, 0xcc6, 0xdcf, 0xec5, 0xfcc,
        0xcc , 0x1c5, 0x2cf, 0x3c6, 0x4ca, 0x5c3, 0x6c9, 0x7c0,
        0x950, 0x859, 0xb53, 0xa5a, 0xd56, 0xc5f, 0xf55, 0xe5c,
        0x15c, 0x55 , 0x35f, 0x256, 0x55a, 0x453, 0x759, 0x650,
        0xaf0, 0xbf9, 0x8f3, 0x9fa, 0xef6, 0xfff, 0xcf5, 0xdfc,
        0x2fc, 0x3f5, 0xff , 0x1f6, 0x6fa, 0x7f3, 0x4f9, 0x5f0,
        0xb60, 0xa69, 0x963, 0x86a, 0xf66, 0xe6f, 0xd65, 0xc6c,
        0x36c, 0x265, 0x16f, 0x66 , 0x76a, 0x663, 0x569, 0x460,
        0xca0, 0xda9, 0xea3, 0xfaa, 0x8a6, 0x9af, 0xaa5, 0xbac,
        0x4ac, 0x5a5, 0x6af, 0x7a6, 0xaa , 0x1a3, 0x2a9, 0x3a0,
        0xd30, 0xc39, 0xf33, 0xe3a, 0x936, 0x83f, 0xb35, 0xa3c,
        0x53c, 0x435, 0x73f, 0x636, 0x13a, 0x33 , 0x339, 0x230,
        0xe90, 0xf99, 0xc93, 0xd9a, 0xa96, 0xb9f, 0x895, 0x99c,
        0x69c, 0x795, 0x49f, 0x596, 0x29a, 0x393, 0x99 , 0x190,
        0xf00, 0xe09, 0xd03, 0xc0a, 0xb06, 0xa0f, 0x905, 0x80c,
        0x70c, 0x605, 0x50f, 0x406, 0x30a, 0x203, 0x109, 0x0   ])
    , triTable = [
        [],
        [0, 8, 3],
        [0, 1, 9],
        [1, 8, 3, 9, 8, 1],
        [1, 2, 10],
        [0, 8, 3, 1, 2, 10],
        [9, 2, 10, 0, 2, 9],
        [2, 8, 3, 2, 10, 8, 10, 9, 8],
        [3, 11, 2],
        [0, 11, 2, 8, 11, 0],
        [1, 9, 0, 2, 3, 11],
        [1, 11, 2, 1, 9, 11, 9, 8, 11],
        [3, 10, 1, 11, 10, 3],
        [0, 10, 1, 0, 8, 10, 8, 11, 10],
        [3, 9, 0, 3, 11, 9, 11, 10, 9],
        [9, 8, 10, 10, 8, 11],
        [4, 7, 8],
        [4, 3, 0, 7, 3, 4],
        [0, 1, 9, 8, 4, 7],
        [4, 1, 9, 4, 7, 1, 7, 3, 1],
        [1, 2, 10, 8, 4, 7],
        [3, 4, 7, 3, 0, 4, 1, 2, 10],
        [9, 2, 10, 9, 0, 2, 8, 4, 7],
        [2, 10, 9, 2, 9, 7, 2, 7, 3, 7, 9, 4],
        [8, 4, 7, 3, 11, 2],
        [11, 4, 7, 11, 2, 4, 2, 0, 4],
        [9, 0, 1, 8, 4, 7, 2, 3, 11],
        [4, 7, 11, 9, 4, 11, 9, 11, 2, 9, 2, 1],
        [3, 10, 1, 3, 11, 10, 7, 8, 4],
        [1, 11, 10, 1, 4, 11, 1, 0, 4, 7, 11, 4],
        [4, 7, 8, 9, 0, 11, 9, 11, 10, 11, 0, 3],
        [4, 7, 11, 4, 11, 9, 9, 11, 10],
        [9, 5, 4],
        [9, 5, 4, 0, 8, 3],
        [0, 5, 4, 1, 5, 0],
        [8, 5, 4, 8, 3, 5, 3, 1, 5],
        [1, 2, 10, 9, 5, 4],
        [3, 0, 8, 1, 2, 10, 4, 9, 5],
        [5, 2, 10, 5, 4, 2, 4, 0, 2],
        [2, 10, 5, 3, 2, 5, 3, 5, 4, 3, 4, 8],
        [9, 5, 4, 2, 3, 11],
        [0, 11, 2, 0, 8, 11, 4, 9, 5],
        [0, 5, 4, 0, 1, 5, 2, 3, 11],
        [2, 1, 5, 2, 5, 8, 2, 8, 11, 4, 8, 5],
        [10, 3, 11, 10, 1, 3, 9, 5, 4],
        [4, 9, 5, 0, 8, 1, 8, 10, 1, 8, 11, 10],
        [5, 4, 0, 5, 0, 11, 5, 11, 10, 11, 0, 3],
        [5, 4, 8, 5, 8, 10, 10, 8, 11],
        [9, 7, 8, 5, 7, 9],
        [9, 3, 0, 9, 5, 3, 5, 7, 3],
        [0, 7, 8, 0, 1, 7, 1, 5, 7],
        [1, 5, 3, 3, 5, 7],
        [9, 7, 8, 9, 5, 7, 10, 1, 2],
        [10, 1, 2, 9, 5, 0, 5, 3, 0, 5, 7, 3],
        [8, 0, 2, 8, 2, 5, 8, 5, 7, 10, 5, 2],
        [2, 10, 5, 2, 5, 3, 3, 5, 7],
        [7, 9, 5, 7, 8, 9, 3, 11, 2],
        [9, 5, 7, 9, 7, 2, 9, 2, 0, 2, 7, 11],
        [2, 3, 11, 0, 1, 8, 1, 7, 8, 1, 5, 7],
        [11, 2, 1, 11, 1, 7, 7, 1, 5],
        [9, 5, 8, 8, 5, 7, 10, 1, 3, 10, 3, 11],
        [5, 7, 0, 5, 0, 9, 7, 11, 0, 1, 0, 10, 11, 10, 0],
        [11, 10, 0, 11, 0, 3, 10, 5, 0, 8, 0, 7, 5, 7, 0],
        [11, 10, 5, 7, 11, 5],
        [10, 6, 5],
        [0, 8, 3, 5, 10, 6],
        [9, 0, 1, 5, 10, 6],
        [1, 8, 3, 1, 9, 8, 5, 10, 6],
        [1, 6, 5, 2, 6, 1],
        [1, 6, 5, 1, 2, 6, 3, 0, 8],
        [9, 6, 5, 9, 0, 6, 0, 2, 6],
        [5, 9, 8, 5, 8, 2, 5, 2, 6, 3, 2, 8],
        [2, 3, 11, 10, 6, 5],
        [11, 0, 8, 11, 2, 0, 10, 6, 5],
        [0, 1, 9, 2, 3, 11, 5, 10, 6],
        [5, 10, 6, 1, 9, 2, 9, 11, 2, 9, 8, 11],
        [6, 3, 11, 6, 5, 3, 5, 1, 3],
        [0, 8, 11, 0, 11, 5, 0, 5, 1, 5, 11, 6],
        [3, 11, 6, 0, 3, 6, 0, 6, 5, 0, 5, 9],
        [6, 5, 9, 6, 9, 11, 11, 9, 8],
        [5, 10, 6, 4, 7, 8],
        [4, 3, 0, 4, 7, 3, 6, 5, 10],
        [1, 9, 0, 5, 10, 6, 8, 4, 7],
        [10, 6, 5, 1, 9, 7, 1, 7, 3, 7, 9, 4],
        [6, 1, 2, 6, 5, 1, 4, 7, 8],
        [1, 2, 5, 5, 2, 6, 3, 0, 4, 3, 4, 7],
        [8, 4, 7, 9, 0, 5, 0, 6, 5, 0, 2, 6],
        [7, 3, 9, 7, 9, 4, 3, 2, 9, 5, 9, 6, 2, 6, 9],
        [3, 11, 2, 7, 8, 4, 10, 6, 5],
        [5, 10, 6, 4, 7, 2, 4, 2, 0, 2, 7, 11],
        [0, 1, 9, 4, 7, 8, 2, 3, 11, 5, 10, 6],
        [9, 2, 1, 9, 11, 2, 9, 4, 11, 7, 11, 4, 5, 10, 6],
        [8, 4, 7, 3, 11, 5, 3, 5, 1, 5, 11, 6],
        [5, 1, 11, 5, 11, 6, 1, 0, 11, 7, 11, 4, 0, 4, 11],
        [0, 5, 9, 0, 6, 5, 0, 3, 6, 11, 6, 3, 8, 4, 7],
        [6, 5, 9, 6, 9, 11, 4, 7, 9, 7, 11, 9],
        [10, 4, 9, 6, 4, 10],
        [4, 10, 6, 4, 9, 10, 0, 8, 3],
        [10, 0, 1, 10, 6, 0, 6, 4, 0],
        [8, 3, 1, 8, 1, 6, 8, 6, 4, 6, 1, 10],
        [1, 4, 9, 1, 2, 4, 2, 6, 4],
        [3, 0, 8, 1, 2, 9, 2, 4, 9, 2, 6, 4],
        [0, 2, 4, 4, 2, 6],
        [8, 3, 2, 8, 2, 4, 4, 2, 6],
        [10, 4, 9, 10, 6, 4, 11, 2, 3],
        [0, 8, 2, 2, 8, 11, 4, 9, 10, 4, 10, 6],
        [3, 11, 2, 0, 1, 6, 0, 6, 4, 6, 1, 10],
        [6, 4, 1, 6, 1, 10, 4, 8, 1, 2, 1, 11, 8, 11, 1],
        [9, 6, 4, 9, 3, 6, 9, 1, 3, 11, 6, 3],
        [8, 11, 1, 8, 1, 0, 11, 6, 1, 9, 1, 4, 6, 4, 1],
        [3, 11, 6, 3, 6, 0, 0, 6, 4],
        [6, 4, 8, 11, 6, 8],
        [7, 10, 6, 7, 8, 10, 8, 9, 10],
        [0, 7, 3, 0, 10, 7, 0, 9, 10, 6, 7, 10],
        [10, 6, 7, 1, 10, 7, 1, 7, 8, 1, 8, 0],
        [10, 6, 7, 10, 7, 1, 1, 7, 3],
        [1, 2, 6, 1, 6, 8, 1, 8, 9, 8, 6, 7],
        [2, 6, 9, 2, 9, 1, 6, 7, 9, 0, 9, 3, 7, 3, 9],
        [7, 8, 0, 7, 0, 6, 6, 0, 2],
        [7, 3, 2, 6, 7, 2],
        [2, 3, 11, 10, 6, 8, 10, 8, 9, 8, 6, 7],
        [2, 0, 7, 2, 7, 11, 0, 9, 7, 6, 7, 10, 9, 10, 7],
        [1, 8, 0, 1, 7, 8, 1, 10, 7, 6, 7, 10, 2, 3, 11],
        [11, 2, 1, 11, 1, 7, 10, 6, 1, 6, 7, 1],
        [8, 9, 6, 8, 6, 7, 9, 1, 6, 11, 6, 3, 1, 3, 6],
        [0, 9, 1, 11, 6, 7],
        [7, 8, 0, 7, 0, 6, 3, 11, 0, 11, 6, 0],
        [7, 11, 6],
        [7, 6, 11],
        [3, 0, 8, 11, 7, 6],
        [0, 1, 9, 11, 7, 6],
        [8, 1, 9, 8, 3, 1, 11, 7, 6],
        [10, 1, 2, 6, 11, 7],
        [1, 2, 10, 3, 0, 8, 6, 11, 7],
        [2, 9, 0, 2, 10, 9, 6, 11, 7],
        [6, 11, 7, 2, 10, 3, 10, 8, 3, 10, 9, 8],
        [7, 2, 3, 6, 2, 7],
        [7, 0, 8, 7, 6, 0, 6, 2, 0],
        [2, 7, 6, 2, 3, 7, 0, 1, 9],
        [1, 6, 2, 1, 8, 6, 1, 9, 8, 8, 7, 6],
        [10, 7, 6, 10, 1, 7, 1, 3, 7],
        [10, 7, 6, 1, 7, 10, 1, 8, 7, 1, 0, 8],
        [0, 3, 7, 0, 7, 10, 0, 10, 9, 6, 10, 7],
        [7, 6, 10, 7, 10, 8, 8, 10, 9],
        [6, 8, 4, 11, 8, 6],
        [3, 6, 11, 3, 0, 6, 0, 4, 6],
        [8, 6, 11, 8, 4, 6, 9, 0, 1],
        [9, 4, 6, 9, 6, 3, 9, 3, 1, 11, 3, 6],
        [6, 8, 4, 6, 11, 8, 2, 10, 1],
        [1, 2, 10, 3, 0, 11, 0, 6, 11, 0, 4, 6],
        [4, 11, 8, 4, 6, 11, 0, 2, 9, 2, 10, 9],
        [10, 9, 3, 10, 3, 2, 9, 4, 3, 11, 3, 6, 4, 6, 3],
        [8, 2, 3, 8, 4, 2, 4, 6, 2],
        [0, 4, 2, 4, 6, 2],
        [1, 9, 0, 2, 3, 4, 2, 4, 6, 4, 3, 8],
        [1, 9, 4, 1, 4, 2, 2, 4, 6],
        [8, 1, 3, 8, 6, 1, 8, 4, 6, 6, 10, 1],
        [10, 1, 0, 10, 0, 6, 6, 0, 4],
        [4, 6, 3, 4, 3, 8, 6, 10, 3, 0, 3, 9, 10, 9, 3],
        [10, 9, 4, 6, 10, 4],
        [4, 9, 5, 7, 6, 11],
        [0, 8, 3, 4, 9, 5, 11, 7, 6],
        [5, 0, 1, 5, 4, 0, 7, 6, 11],
        [11, 7, 6, 8, 3, 4, 3, 5, 4, 3, 1, 5],
        [9, 5, 4, 10, 1, 2, 7, 6, 11],
        [6, 11, 7, 1, 2, 10, 0, 8, 3, 4, 9, 5],
        [7, 6, 11, 5, 4, 10, 4, 2, 10, 4, 0, 2],
        [3, 4, 8, 3, 5, 4, 3, 2, 5, 10, 5, 2, 11, 7, 6],
        [7, 2, 3, 7, 6, 2, 5, 4, 9],
        [9, 5, 4, 0, 8, 6, 0, 6, 2, 6, 8, 7],
        [3, 6, 2, 3, 7, 6, 1, 5, 0, 5, 4, 0],
        [6, 2, 8, 6, 8, 7, 2, 1, 8, 4, 8, 5, 1, 5, 8],
        [9, 5, 4, 10, 1, 6, 1, 7, 6, 1, 3, 7],
        [1, 6, 10, 1, 7, 6, 1, 0, 7, 8, 7, 0, 9, 5, 4],
        [4, 0, 10, 4, 10, 5, 0, 3, 10, 6, 10, 7, 3, 7, 10],
        [7, 6, 10, 7, 10, 8, 5, 4, 10, 4, 8, 10],
        [6, 9, 5, 6, 11, 9, 11, 8, 9],
        [3, 6, 11, 0, 6, 3, 0, 5, 6, 0, 9, 5],
        [0, 11, 8, 0, 5, 11, 0, 1, 5, 5, 6, 11],
        [6, 11, 3, 6, 3, 5, 5, 3, 1],
        [1, 2, 10, 9, 5, 11, 9, 11, 8, 11, 5, 6],
        [0, 11, 3, 0, 6, 11, 0, 9, 6, 5, 6, 9, 1, 2, 10],
        [11, 8, 5, 11, 5, 6, 8, 0, 5, 10, 5, 2, 0, 2, 5],
        [6, 11, 3, 6, 3, 5, 2, 10, 3, 10, 5, 3],
        [5, 8, 9, 5, 2, 8, 5, 6, 2, 3, 8, 2],
        [9, 5, 6, 9, 6, 0, 0, 6, 2],
        [1, 5, 8, 1, 8, 0, 5, 6, 8, 3, 8, 2, 6, 2, 8],
        [1, 5, 6, 2, 1, 6],
        [1, 3, 6, 1, 6, 10, 3, 8, 6, 5, 6, 9, 8, 9, 6],
        [10, 1, 0, 10, 0, 6, 9, 5, 0, 5, 6, 0],
        [0, 3, 8, 5, 6, 10],
        [10, 5, 6],
        [11, 5, 10, 7, 5, 11],
        [11, 5, 10, 11, 7, 5, 8, 3, 0],
        [5, 11, 7, 5, 10, 11, 1, 9, 0],
        [10, 7, 5, 10, 11, 7, 9, 8, 1, 8, 3, 1],
        [11, 1, 2, 11, 7, 1, 7, 5, 1],
        [0, 8, 3, 1, 2, 7, 1, 7, 5, 7, 2, 11],
        [9, 7, 5, 9, 2, 7, 9, 0, 2, 2, 11, 7],
        [7, 5, 2, 7, 2, 11, 5, 9, 2, 3, 2, 8, 9, 8, 2],
        [2, 5, 10, 2, 3, 5, 3, 7, 5],
        [8, 2, 0, 8, 5, 2, 8, 7, 5, 10, 2, 5],
        [9, 0, 1, 5, 10, 3, 5, 3, 7, 3, 10, 2],
        [9, 8, 2, 9, 2, 1, 8, 7, 2, 10, 2, 5, 7, 5, 2],
        [1, 3, 5, 3, 7, 5],
        [0, 8, 7, 0, 7, 1, 1, 7, 5],
        [9, 0, 3, 9, 3, 5, 5, 3, 7],
        [9, 8, 7, 5, 9, 7],
        [5, 8, 4, 5, 10, 8, 10, 11, 8],
        [5, 0, 4, 5, 11, 0, 5, 10, 11, 11, 3, 0],
        [0, 1, 9, 8, 4, 10, 8, 10, 11, 10, 4, 5],
        [10, 11, 4, 10, 4, 5, 11, 3, 4, 9, 4, 1, 3, 1, 4],
        [2, 5, 1, 2, 8, 5, 2, 11, 8, 4, 5, 8],
        [0, 4, 11, 0, 11, 3, 4, 5, 11, 2, 11, 1, 5, 1, 11],
        [0, 2, 5, 0, 5, 9, 2, 11, 5, 4, 5, 8, 11, 8, 5],
        [9, 4, 5, 2, 11, 3],
        [2, 5, 10, 3, 5, 2, 3, 4, 5, 3, 8, 4],
        [5, 10, 2, 5, 2, 4, 4, 2, 0],
        [3, 10, 2, 3, 5, 10, 3, 8, 5, 4, 5, 8, 0, 1, 9],
        [5, 10, 2, 5, 2, 4, 1, 9, 2, 9, 4, 2],
        [8, 4, 5, 8, 5, 3, 3, 5, 1],
        [0, 4, 5, 1, 0, 5],
        [8, 4, 5, 8, 5, 3, 9, 0, 5, 0, 3, 5],
        [9, 4, 5],
        [4, 11, 7, 4, 9, 11, 9, 10, 11],
        [0, 8, 3, 4, 9, 7, 9, 11, 7, 9, 10, 11],
        [1, 10, 11, 1, 11, 4, 1, 4, 0, 7, 4, 11],
        [3, 1, 4, 3, 4, 8, 1, 10, 4, 7, 4, 11, 10, 11, 4],
        [4, 11, 7, 9, 11, 4, 9, 2, 11, 9, 1, 2],
        [9, 7, 4, 9, 11, 7, 9, 1, 11, 2, 11, 1, 0, 8, 3],
        [11, 7, 4, 11, 4, 2, 2, 4, 0],
        [11, 7, 4, 11, 4, 2, 8, 3, 4, 3, 2, 4],
        [2, 9, 10, 2, 7, 9, 2, 3, 7, 7, 4, 9],
        [9, 10, 7, 9, 7, 4, 10, 2, 7, 8, 7, 0, 2, 0, 7],
        [3, 7, 10, 3, 10, 2, 7, 4, 10, 1, 10, 0, 4, 0, 10],
        [1, 10, 2, 8, 7, 4],
        [4, 9, 1, 4, 1, 7, 7, 1, 3],
        [4, 9, 1, 4, 1, 7, 0, 8, 1, 8, 7, 1],
        [4, 0, 3, 7, 4, 3],
        [4, 8, 7],
        [9, 10, 8, 10, 11, 8],
        [3, 0, 9, 3, 9, 11, 11, 9, 10],
        [0, 1, 10, 0, 10, 8, 8, 10, 11],
        [3, 1, 10, 11, 3, 10],
        [1, 2, 11, 1, 11, 9, 9, 11, 8],
        [3, 0, 9, 3, 9, 11, 1, 2, 9, 2, 11, 9],
        [0, 2, 11, 8, 0, 11],
        [3, 2, 11],
        [2, 3, 8, 2, 8, 10, 10, 8, 9],
        [9, 10, 2, 0, 9, 2],
        [2, 3, 8, 2, 8, 10, 0, 1, 8, 1, 10, 8],
        [1, 10, 2],
        [1, 3, 8, 9, 1, 8],
        [0, 9, 1],
        [0, 3, 8],
        []]
    , cubeVerts = [
        [0,0,0]
        ,[1,0,0]
        ,[1,1,0]
        ,[0,1,0]
        ,[0,0,1]
        ,[1,0,1]
        ,[1,1,1]
        ,[0,1,1]]
    , edgeIndex = [ [0,1],[1,2],[2,3],[3,0],[4,5],[5,6],[6,7],[7,4],[0,4],[1,5],[2,6],[3,7] ];



    return function(dims, potential, bounds) {
        if(!bounds) {
            bounds = [[0,0,0], dims];
        }
        var scale     = [0,0,0];
        var shift     = [0,0,0];
        for(var i=0; i<3; ++i) {
            scale[i] = (bounds[1][i] - bounds[0][i]) / dims[i];
            shift[i] = bounds[0][i];
        }

        var vertices = []
        , faces = []
        , n = 0
        , grid = new Array(8)
        , edges = new Array(12)
        , x = [0,0,0];
        //March over the volume
        for(x[2]=0; x[2]<dims[2]-1; ++x[2], n+=dims[0])
            for(x[1]=0; x[1]<dims[1]-1; ++x[1], ++n)
                for(x[0]=0; x[0]<dims[0]-1; ++x[0], ++n) {
                    //For each cell, compute cube mask
                    var cube_index = 0;
                    for(var i=0; i<8; ++i) {
                        var v = cubeVerts[i]
                        , s = potential(
                            scale[0]*(x[0]+v[0])+shift[0],
                            scale[1]*(x[1]+v[1])+shift[1],
                            scale[2]*(x[2]+v[2])+shift[2]);
                        grid[i] = s;
                        cube_index |= (s > 0) ? 1 << i : 0;
                    }
                    //Compute vertices
                    var edge_mask = edgeTable[cube_index];
                    if(edge_mask === 0) {
                        continue;
                    }
                    for(var i=0; i<12; ++i) {
                        if((edge_mask & (1<<i)) === 0) {
                            continue;
                        }
                        edges[i] = vertices.length;
                        var nv = [0,0,0]
                        , e = edgeIndex[i]
                        , p0 = cubeVerts[e[0]]
                        , p1 = cubeVerts[e[1]]
                        , a = grid[e[0]]
                        , b = grid[e[1]]
                        , d = a - b
                        , t = 0;
                        if(Math.abs(d) > 1e-6) {
                            t = a / d;
                        }
                        for(var j=0; j<3; ++j) {
                            nv[j] = scale[j] * ((x[j] + p0[j]) + t * (p1[j] - p0[j])) + shift[j];
                        }
                        vertices.push(nv);
                    }
                    //Add faces
                    var f = triTable[cube_index];
                    for(var i=0; i<f.length; i += 3) {
                        faces.push([edges[f[i]], edges[f[i+1]], edges[f[i+2]]]);
                    }
                }
        return { positions: vertices, cells: faces };
    };

}
