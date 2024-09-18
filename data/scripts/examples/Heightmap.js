// LibreSprite
// Copyright (C) 2024 LibreSprite contributors

var dialog, intSeed;

const tweakRain = false;

var FAt = 0.05, CAt = 0.1, speedAt = 0.75, DAt = 0.09, LT = 20, FR = 0.8;
var Blend = 50;

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

    for (let ix = 0; ix < cycles; ++ix) {
        let y = Math.random() * H;
        let x = Math.random() * W;
        let w = 0;
        let vx = FAt * (Math.random() * 2 - 1);
        let vy = FAt * (Math.random() * 2 - 1);

        for (let i = 0; i < LT; ++i) {
            let py = y|0;
            let px = x|0;

            let s = Math.sqrt(vx*vx + vy*vy);
            if (s) {
                y += vy/s;
                x += vx/s;
                data[py * W + px] -= s * CAt;
                data[(y|0) * W + (x|0)] += s * DAt;
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

    /* blur
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
    */

    for (let i = 0; i < data.length; ++i) {
        let v = data[i];        // 0 ... 1
        if (v < 0) v = 0;
        else if (v > 1) v = 1;
        data[i] = v;
    }
    return data;
    function rng(x, y) {
        return (((((X + x) * 1991 ^ (Y + y) * 2731) * seed) >>> 0) & 0xFFFF) / 0xFFFF;
    }
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

function terrain(X, Y, W, H, seed, cycles) {
    const heightmap = vectorSigma(
        cloud(X, Y, W, H, seed),
        scale(sigma(cloud(X, Y, W, H, seed + 85821), 3), 3)
    );
    const erosionmap = rain(heightmap, W, H, seed, cycles);
    vectorHardLight(heightmap, erosionmap, Blend / 100);
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
        dialog.addBreak();
        dialog.addEntry("Y:", "intY").value = "0";
        dialog.addBreak();
        dialog.addEntry("Erosion (0-9):", "intRain").value = storage.get("intRain") || "8";
        dialog.addBreak();
        dialog.addEntry("Blending (0-100):", "intBlend").value = storage.get("intBlend") || "50";

        if (tweakRain) {
            dialog.addEntry("F", "intF").value = storage.get("intF") || "0.5";
            dialog.addEntry("C", "intC").value = storage.get("intC") || "0.1";
            dialog.addEntry("S", "intS").value = storage.get("intS") || "0.3";
            dialog.addBreak();
            dialog.addEntry("D", "intD").value = storage.get("intD") || "0.15";
            dialog.addEntry("LT", "intLT").value = storage.get("intLT") || "10";
            dialog.addEntry("FR", "intFR").value = storage.get("intFR") || "0.8";
        } else {
            storage.set("0.05", "intF");
            storage.set("0.1", "intC");
            storage.set("0.75", "intS");
            storage.set("0.09", "intD");
            storage.set("20", "intLT");
            storage.set("0.8", "intFR");
        }
        dialog.addBreak();
        dialog.addButton("Run", "run");
    },

    dlgHeightmap_close:function(){
        dialog = null;
        intSeed = null;
    },

    btnRand_click:function(){
        intSeed.value = "" + (Math.random() * 0xFFFF | 0);
        eventHandlers.run_click();
    },

    intSeed_change:function(){}, // eventHandlers.run_click();},
    intX_change:function(){}, // eventHandlers.run_click();},
    intY_change:function(){}, // eventHandlers.run_click();},
    intF_change:function(){eventHandlers.run_click();},
    intC_change:function(){eventHandlers.run_click();},
    intS_change:function(){eventHandlers.run_click();},
    intD_change:function(){eventHandlers.run_click();},
    intLT_change:function(){eventHandlers.run_click();},
    intFR_change:function(){eventHandlers.run_click();},
    intRain_change:function(){},// eventHandlers.run_click();},
    intBlend_change:function(){},// eventHandlers.run_click();},

    run_click:function(){
        let seed = storage.get("intSeed")|0;
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
    else
        console.log(eventName, JSON.stringify(args));
}
