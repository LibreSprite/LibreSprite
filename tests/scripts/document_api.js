// Headless DocumentApi layer/frame binding test.
// Run from the LibreSprite source directory with:
//   libresprite -b --script tests/scripts/document_api.js

const input = "data/splash.ase";
const output = "build-codex-ucrt/document-api-test.ase";

function assert(condition, message) {
  if (!condition)
    throw new Error(message);
}

const opened = app.open(input);
assert(!!opened, "app.open() did not return a document");

const initialLayers = sprite.layerCount;
const initialFrames = sprite.frameCount;
const createdLayer = sprite.newLayer("MCP Layer");
assert(!!createdLayer, "newLayer() did not return a Layer wrapper");
assert(createdLayer.name === "MCP Layer", "new layer name is wrong");
assert(sprite.layerCount === initialLayers + 1, "layer count did not increase");

const copiedFrame = sprite.addFrame();
assert(copiedFrame === initialFrames, "addFrame() returned the wrong index");
assert(sprite.frameCount === initialFrames + 1, "frame count did not increase after addFrame()");

const emptyFrameIndex = sprite.frameCount;
const emptyFrame = sprite.addEmptyFrame(emptyFrameIndex);
assert(emptyFrame === emptyFrameIndex, "addEmptyFrame() returned the wrong index");
assert(sprite.frameCount === initialFrames + 2, "frame count did not increase after addEmptyFrame()");

sprite.saveAs(output, true);
const reopened = app.open(output);
assert(!!reopened, "saved sprite could not be reopened");
assert(sprite.layerCount === initialLayers + 1, "layer count was not persisted");
assert(sprite.frameCount === initialFrames + 2, "frame count was not persisted");

let foundLayer = false;
for (let i = 0; i < sprite.layerCount; ++i) {
  if (sprite.layer(i).name === "MCP Layer")
    foundLayer = true;
}
assert(foundLayer, "new layer was not persisted");
console.log("DocumentApi layer/frame bindings: PASS");
