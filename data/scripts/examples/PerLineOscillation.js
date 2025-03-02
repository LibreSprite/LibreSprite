// LibreSprite
// Copyright (C) 2024 LibreSprite contributors

var dialog;

function wrap(x, n) {
    if (x < 0)
	return ((x % n + n) % n) | 0;
    return ((x + n) % n) | 0;
}

const effects = [
    // horizontal oscillation
    function(src, angle, width, height, scale) {
	const out = new Uint8Array(src.length);
	for (var y = 0; y < height; ++y) {
            var ox = Math.sin(y / scale + angle * Math.PI * 2) * scale | 0;
            for (var x = 0; x < width; ++x) {
		var oi = (y * width + x) * 4;
		var ii = (y * width + wrap(x + ox, width)) * 4;
		out[oi++] = src[ii++];
		out[oi++] = src[ii++];
		out[oi++] = src[ii++];
		out[oi++] = src[ii++];
            }
	}
	return out;
    },

    // same as above, but with anti-aliasing
    function(src, angle, width, height, scale) {
	const out = new Uint8Array(src.length);
	for (var y = 0; y < height; ++y) {
            var ox = Math.sin(y / scale + angle * Math.PI * 2) * scale;
	    var ox0 = ox | 0;
	    var ox1 = ox0 + 1;
	    var a = ox - ox0;
            for (var x = 0; x < width; ++x) {
		var oi = (y * width + x) * 4;
		var ii0 = (y * width + wrap(x + ox0, width)) * 4;
		var ii1 = (y * width + wrap(x + ox1, width)) * 4;

		out[oi++] = src[ii0++] * (1 - a) + src[ii1++] * a;
		out[oi++] = src[ii0++] * (1 - a) + src[ii1++] * a;
		out[oi++] = src[ii0++] * (1 - a) + src[ii1++] * a;
		out[oi++] = src[ii0++] * (1 - a) + src[ii1++] * a;
            }
	}
	return out;
    },

    // horizontal oscillation, RGB desync'd
    function(src, angle, width, height, scale) {
	const out = new Uint8Array(src.length);
	for (var y = 0; y < height; ++y) {
            for (var x = 0; x < width; ++x) {
		var d = 0;
		var oi = (y * width + x) * 4;
		var ox = Math.sin(y / scale + (angle + d) * Math.PI * 2) * scale | 0;
		var ii = (y * width + wrap(x + ox, width)) * 4;
		out[oi++] = src[ii++];

		d += 0.03;

		ox = Math.sin(y / scale + (angle + d) * Math.PI * 2) * scale | 0;
		ii = (y * width + wrap(x + ox, width)) * 4 + 1;
		out[oi++] = src[ii++];

		ox = Math.sin(y / scale + (angle + d) * Math.PI * 2) * scale | 0;
		ii = (y * width + wrap(x + ox, width)) * 4 + 2;
		out[oi++] = src[ii++];

		ox = Math.sin(y / scale + (angle + d) * Math.PI * 2) * scale | 0;
		ii = (y * width + wrap(x + ox, width)) * 4 + 3;
		out[oi++] = src[ii++];
            }
	}
	return out;
    },

    // vertical oscillation
    function(src, angle, width, height, scale) {
	const out = new Uint8Array(src.length);
	for (var y = 0; y < height; ++y) {
            var oy = Math.sin(y / scale + angle * Math.PI * 2) * scale | 0;
            for (var x = 0; x < width; ++x) {
		var oi = (y * width + x) * 4;
		var ii = (wrap(y + oy, height) * width + x) * 4;
		out[oi++] = src[ii++];
		out[oi++] = src[ii++];
		out[oi++] = src[ii++];
		out[oi++] = src[ii++];
            }
	}
	return out;
    },

    // same as above, but with anti-aliasing
    function(src, angle, width, height, scale) {
	const out = new Uint8Array(src.length);
	for (var y = 0; y < height; ++y) {
            var oy = y + Math.sin(y / scale + angle * Math.PI * 2) * scale;
	    var oy0 = wrap(oy | 0, height) * width;
	    var ny = (y + 1) + Math.sin((y + 1) / scale + angle * Math.PI * 2) * scale;
	    var oy1 = wrap(ny | 0, height) * width;
	    var a = oy - Math.floor(oy);
            for (var x = 0; x < width; ++x) {
		var oi = (y * width + x) * 4;
		var ii0 = (oy0 + x) * 4;
		var ii1 = (oy1 + x) * 4;

		out[oi++] = src[ii0++] * (1 - a) + src[ii1++] * a;
		out[oi++] = src[ii0++] * (1 - a) + src[ii1++] * a;
		out[oi++] = src[ii0++] * (1 - a) + src[ii1++] * a;
		out[oi++] = src[ii0++] * (1 - a) + src[ii1++] * a;
            }
	}
	return out;
    }
];

const eventHandlers = {
    init:function(){
        if (!app.activeSprite) {
            app.createDialog('Error').addLabel('Need an image to oscillate.');
            return;
        }

        if (dialog)
            dialog.close();
        dialog = app.createDialog('dialog');
        dialog.addIntEntry("Frames to animate:", "frameCount", 0, 100);
        dialog.addBreak();
        dialog.addIntEntry("Wave size:", "waveSize", 3, 100);
        dialog.addBreak();
        dialog.addIntEntry("Effect:", "effect", 0, effects.length - 1);
        dialog.addBreak();
        dialog.addButton("Run", "run");
    },

    run_click:function(){
        dialog.close();
        dialog = null;

	app.command.setParameter("format", "rgb");
	app.command.ChangePixelFormat();
	app.command.clearParameters();

        const frameCount = storage.get("frameCount")|0;
        const waveSize = storage.get("waveSize")|0;
	const effect = storage.get("effect")|0;
	const oscillate = effects[effect] || effects[0];
        const sprite = app.activeSprite;
        const layerNumber = app.activeLayerNumber;
        const reference = sprite.layer(layerNumber).cel(0).image;
        const refWidth = reference.width;
        const refHeight = reference.height;
        const src = reference.getImageData();

        app.command.setParameter("content", "current");
        for (var i = 0; i < frameCount; ++i) {
            if (i)
                app.command.NewFrame();
            sprite.layer(layerNumber).cel(i).image.putImageData(oscillate(src, i / frameCount, refWidth, refHeight, waveSize));
        }
    }
};

function onEvent(eventName) {
    var handler = eventHandlers[eventName];
    if (typeof handler == 'function')
        handler();
}
