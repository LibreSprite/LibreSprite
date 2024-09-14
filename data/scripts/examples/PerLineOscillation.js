// LibreSprite
// Copyright (C) 2024 LibreSprite contributors

var dialog;

function oscillate(src, angle, width, height, scale) {
    const out = new Uint8Array(src.length);
    for (var y = 0; y < height; ++y) {
        var ox = Math.sin(y / scale + angle * Math.PI * 2) * scale | 0;
        for (var x = 0; x < width; ++x) {
            var oi = (y * width + x) * 4;
            var ii = (y * width + (x + ox) % width) * 4;
            out[oi++] = src[ii++];
            out[oi++] = src[ii++];
            out[oi++] = src[ii++];
            out[oi++] = src[ii++];
        }
    }
    return out;
}

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
        dialog.addButton("Run", "run");
    },

    run_click:function(){
        dialog.close();
        dialog = null;

        const frameCount = storage.get("frameCount")|0;
        const waveSize = storage.get("waveSize")|0;
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
