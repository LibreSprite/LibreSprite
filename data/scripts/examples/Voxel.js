// LibreSprite
// Copyright (C) 2024 LibreSprite contributors

let onEvent;

class UI {
    width = 320;
    height = 320;
    framebuffer = null;
    voxels = null;

    scheduled = false;
    speedX = 0;
    speedY = 0;
    rotationY = 0;
    rotationX = 0;

    constructor() {
        onEvent = this.onEvent.bind(this);

        this.dlg = app.createDialog("dlgVoxel");
        this.dlg.addButton("Update", "update");
        this.dlg.addButton("Export", "export");
        this.dlg.addButton("H", "setH");
        this.dlg.addButton("M", "setM");
        this.dlg.addButton("L", "setL");
        this.dlg.addButton("<", "addY");
        this.dlg.addButton(">", "subY");
        this.dlg.addBreak();
        this.image = this.dlg.addImageView("image");
        this.framebuffer = new Uint8Array(this.width * this.height * 4);
        this.update();
    }

    init(){}

    setH(){this.rotationX = 0.5;}
    setM(){this.rotationX = 0.0;}
    setL(){this.rotationX = -0.5;}
    subY(){this.speedY -= 0.025;}
    addY(){this.speedY += 0.025;}

    dlgVoxel = {
        close(){
            this.dlg = null;
            this.image = null;
        },

        resize(width, height){
            height -= 64;
            if (height <= 0)
                return;
            if (this.width == width && this.height == height && this.framebuffer)
                return;
            this.width = width;
            this.height = height;
            this.framebuffer = new Uint8Array(width * height * 4);
            if (!this.scheduled)
                this.render();
        }
    };

    export(){
        if (!this.rawMesh)
            return;

        let {positions, cells, scale} = this.rawMesh;
        const fileName = `voxel`;
        storage.set(STLBIN(), 'stl', fileName);
        let path = storage.save('stl', fileName);
        this.dlg.title = path ? path : "Could not save " + fileName;
        if (path) app.launch(path);

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
                        Float[0] = positions[i][j] / scale;
                        src.push(...Bytes);
                    }
                }
                src.push(0, 0);
            }
            return Uint8Array.from(src);
        }
    }

    update(){
        this.rebuild();
        if (!this.scheduled)
            this.render();
    }

    rebuild(){
        const sprite = app.activeSprite;
        const frame = app.activeFrameNumber|0;
        const width = sprite?.width|0;
        const height = sprite?.layerCount|0;
        const depth = sprite?.height|0;
        const voxels = this.voxels = new Array(width * height * depth);

        for (let l = 0; l < height; ++l) {
            const cel = sprite.layer(l).cel(frame);
            if (!cel)
                continue;
            const ix = cel.x;
            const iy = cel.y;
            const image = cel.image;
            if (!image)
                continue;
            const iw = image.width;
            const ih = image.height;
            let data = image.getImageData();
            for (let y = 0; y < ih; ++y) {
                for (let x = 0; x < iw; ++x) {
                    let i = (y * iw + x) * 4;
                    let j = l * depth * width + (iy + y) * width + x + ix;
                    let c = [data[i], data[i+1], data[i+2], data[i+3]];
                    if (c[3] > 128) {
                        voxels[j] = (c[3] << 24) | (c[2] << 16) | (c[1] << 8) | (c[0] << 0);
                    }
                }
            }
        }
        const mc = getMarchingCubes();
        const {positions, cells} = mc([width + 2, height + 2, depth + 2], (x, y, z) => {
            x--; y--; z--;
            if (x < 0 || y < 0 || z < 0 || x >= width || y >= height || z >= depth)
                return [0, 0];
            let c = voxels[(y|0) * depth * width + (z|0) * width + (x|0)];
            return [!!c, c];
        });

        this.rawMesh = {positions, cells};

        this.mesh = makeMesh.call(this);

        function makeMesh() {
            let tris = [];
            let minX = positions[0][0];
            let minY = positions[0][1];
            let minZ = positions[0][2];
            let maxX = positions[0][0];
            let maxY = positions[0][1];
            let maxZ = positions[0][2];

            for (let pos of positions) {
                if (minX > pos[0]) minX = pos[0];
                if (minY > pos[1]) minY = pos[1];
                if (minZ > pos[2]) minZ = pos[2];
                if (maxX < pos[0]) maxX = pos[0];
                if (maxY < pos[1]) maxY = pos[1];
                if (maxZ < pos[2]) maxZ = pos[2];
            }

            let centerX = (maxX - minX) / 2;
            let centerY = (maxY - minY) / 2;
            let centerZ = (maxZ - minZ) / 2;
            let scale = 1 / Math.max(maxX - minX, maxY - minY, maxZ - minZ);
            this.rawMesh.scale = scale;

            for (let pos of positions) {
                let x = pos[0];
                let y = pos[1];
                let z = pos[2];
                pos[0] = (x - centerX) * scale;
                pos[1] = (y - centerY) * scale;
                pos[2] = (z - centerZ) * scale;
            }

            for (let face of cells) {
                let p = [positions[face[0]], positions[face[1]], positions[face[2]]];
                let Ax = p[0][0] - p[1][0];
                let Ay = p[0][1] - p[1][1];
                let Az = p[0][2] - p[1][2];
                let Bx = p[0][0] - p[2][0];
                let By = p[0][1] - p[2][1];
                let Bz = p[0][2] - p[2][2];
                let n = [
                    Ay * Bz - Az * By,
                    Az * Bx - Ax * Bz,
                    Ax * By - Ay * Bx
                ];

                let l = Math.sqrt(n[0]*n[0] + n[1]*n[1] + n[2]*n[2]);
                n[0] /= l;
                n[1] /= l;
                n[2] /= l;

                let c = p[0][3];
                if (!c) c = 0xFFFF00FF;
                tris.push({p, n, c:c});
            }
            return tris;
        }
    }

    yield() {
        this.scheduled = false;
        if (this.dlg)
            this.render();
    }

    render(){
        const sY = Math.sin(this.rotationY);
        const cY = Math.cos(this.rotationY);
        this.rotationY += this.speedY;
        const sX = Math.sin(this.rotationX);
        const cX = Math.cos(this.rotationX);
        this.rotationX += this.speedX;

        const W = this.width;
        const H = this.height;
        const FB = new Uint32Array(this.framebuffer.buffer);
        FB.fill(0);

        const queue = [];
        if (this.mesh) {
            process(queue, this.mesh);
            if (!this.scheduled) {
                this.scheduled = true;
                app.yield();
            }
        }
        queue.sort((a, b) => b.z - a.z);
        drawQueue(queue);

        this.image.putImageData(this.framebuffer, W, H);

        function process(queue, m) {
            for (let face of m) {
                let p = [
                    transform(face.p[0]),
                    transform(face.p[1]),
                    transform(face.p[2]),
                ];

                let Ax = p[1][0] - p[0][0];
                let Ay = p[1][1] - p[0][1];
                let Bx = p[2][0] - p[0][0];
                let By = p[2][1] - p[0][1];
                let cross = Ax * By - Ay * Bx;
                if (cross < 0)
                    continue;

                let z = Math.min(p[0][2], p[1][2], p[2][2]);
                queue.push({p, z, n:face.n, c:face.c});
            }
        }

        function transform(v) {
            let xt = (v[0] * cY) - (v[2] * sY);
            let yt = v[1];
            let zt = (v[0] * sY) + (v[2] * cY);

            let x = xt;
            let y = yt * cX - zt * sX;
            let z = yt * sX - zt * cX;

            z += 100;

            let fovz = W * 90 / (90 + z);
            x = (x * fovz) + (W/2);
            y = (H/2) - (y * fovz);

            return [x, y, z];
        }

        function drawQueue(queue) {
            for (let face of queue) {
                let l = 1 - Math.min(1, Math.max(face.n[1] * 0.5 + 0.5, 0)) * 0.75;
                let r = (face.c & 0xFF) * l;
                let g = ((face.c >> 8) & 0xFF) * l;
                let b = ((face.c >> 16) & 0xFF) * l;
                let c = 0xFF000000 | (b << 16) | (g << 8) | (r << 0);

                fillTriangle(face.p[0][0]|0,face.p[0][1]|0,
                             face.p[1][0]|0,face.p[1][1]|0,
                             face.p[2][0]|0,face.p[2][1]|0,
                             c
                            );
            }
        }

        function drawHLine(x, y, w, c) {
            if (y >= H || y < 0 || x > W || x + w < 0)
                return;
            if (x < 0) {
                w += x;
                x = 0;
            }
            if (x + w > W)
                w = W - x;
            if (w <= 0)
                return;
            let j = (y|0) * W + (x|0);
            for (let i = 0; i < w; ++i) {
                FB[j++] = c;
            }
        }

        function fillTriangle(x0, y0, x1,y1, x2,y2, col) {
            let a, b, y, last, tmp;
            a = W;
            b = H;
            if( x0 < 0 && x1 < 0 && x2 < 0 ) return;
            if( x0 >= a && x1 > a && x2 > a ) return;
            if( y0 < 0 && y1 < 0 && y2 < 0 ) return;
            if( y0 >= b && y1 > b && y2 > b ) return;

            // Sort coordinates by Y order (y2 >= y1 >= y0)
            if (y0 > y1) {
                tmp = y0; y0 = y1; y1 = tmp;
                tmp = x0; x0 = x1; x1 = tmp;
            }
            if (y1 > y2) {
                tmp = y2; y2 = y1; y1 = tmp;
                tmp = x2; x2 = x1; x1 = tmp;
            }
            if (y0 > y1) {
                tmp = y0; y0 = y1; y1 = tmp;
                tmp = x0; x0 = x1; x1 = tmp;
            }

            if (y0 == y2) { // Handle awkward all-on-same-line case as its own thing
                a = b = x0;
                if (x1 < a) a = x1;
                else if (x1 > b) b = x1;
                if (x2 < a) a = x2;
                else if (x2 > b) b = x2;
                drawHLine(a, y0, b - a + 1, col);
                return;
            }

            let dx01 = x1 - x0,
            dx02 = x2 - x0,
            dy02 = 1 / (y2 - y0),
            dx12 = x2 - x1,
            sa = 0,
            sb = 0;

            // For upper part of triangle, find scanline crossings for segments
            // 0-1 and 0-2.  If y1=y2 (flat-bottomed triangle), the scanline y1
            // is included here (and second loop will be skipped, avoiding a /0
            // error there), otherwise scanline y1 is skipped here and handled
            // in the second loop...which also avoids a /0 error here if y0=y1
            // (flat-topped triangle).
            if (y1 == y2) last = y1; // Include y1 scanline
            else last = y1 - 1; // Skip it

            y = y0;

            if( y0 != y1 ){
                let dy01 = 1 / (y1 - y0);
                for (y = y0; y <= last; y++) {
                    a = x0 + (sa * dy01)|0;
                    b = x0 + (sb * dy02)|0;
                    sa += dx01;
                    sb += dx02;
                    /* longhand:
                       a = x0 + (x1 - x0) * (y - y0) / (y1 - y0);
                       b = x0 + (x2 - x0) * (y - y0) / (y2 - y0);
                    */
                    if (a > b){
                        tmp = a;
                        a = b;
                        b = tmp;
                    }
                    drawHLine(a, y, b - a + 1, col);
                }
            }

            // For lower part of triangle, find scanline crossings for segments
            // 0-2 and 1-2.  This loop is skipped if y1=y2.
            if( y1 != y2 ){
                let dy12 = 1 / (y2 - y1);
                sa = dx12 * (y - y1);
                sb = dx02 * (y - y0);
                for (; y <= y2; y++) {
                    a = x1 + (sa * dy12)|0;
                    b = x0 + (sb * dy02)|0;
                    sa += dx12;
                    sb += dx02;
                    if (a > b){
                        tmp = a;
                        a = b;
                        b = tmp;
                    }
                    drawHLine(a, y, b - a + 1, col);
                }
            }
        }
    }

    onEvent(eventName, ...args) {
        const parts = eventName.split("_");
        for (let handler of [eventName[this], this[parts[1]], this[parts[0]], this[parts[0]]?.[parts[1]]]) {
            if (typeof handler == 'function') {
                handler.apply(this, args);
                return;
            }
        }
        console.log(eventName, JSON.stringify(args));
    }
}

new UI();

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
        , colors = new Array(8)
        , x = [0,0,0];
        //March over the volume
        for(x[2]=0; x[2]<dims[2]-1; ++x[2], n+=dims[0])
            for(x[1]=0; x[1]<dims[1]-1; ++x[1], ++n)
                for(x[0]=0; x[0]<dims[0]-1; ++x[0], ++n) {
                    //For each cell, compute cube mask
                    var cube_index = 0;
                    for(var i=0; i<8; ++i) {
                        var v = cubeVerts[i];
                        let [s, color] = potential(
                            scale[0]*(x[0]+v[0])+shift[0],
                            scale[1]*(x[1]+v[1])+shift[1],
                            scale[2]*(x[2]+v[2])+shift[2]);
                        grid[i] = s;
                        colors[i] = color;
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
                        var nv = [0,0,0,0]
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
                        nv[3] = colors[e[0]] || colors[e[1]];
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
