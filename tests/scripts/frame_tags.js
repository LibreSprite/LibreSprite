// Headless frame-tag binding test.
// Run from the LibreSprite source directory with:
//   libresprite -b --script tests/scripts/frame_tags.js

const input = "data/splash.ase";
const output = "build-codex-ucrt/frame-tags-test.ase";

function assert(condition, message) {
  if (!condition)
    throw new Error(message);
}

const opened = app.open(input);
assert(!!opened, "app.open() did not return a document");
assert(sprite.tags.length === 0, "fixture must start without frame tags");

const tag = sprite.addTag(0, 0);
tag.name = "Walk";
tag.color = 0xFF3366CC;
tag.setFrameRange(0, 0);
tag.aniDir = "pingpong";

assert(sprite.tags.length === 1, "tag was not added");
assert(sprite.tags[0].name === "Walk", "tag rename failed");
assert(sprite.tags[0].color === 0xFF3366CC, "tag recolor failed");
assert(sprite.tags[0].fromFrame === 0 && sprite.tags[0].toFrame === 0, "tag range failed");
assert(sprite.tags[0].aniDir === "pingpong", "tag animation direction failed");

sprite.saveAs(output, true);
const reopened = app.open(output);
assert(!!reopened, "saved sprite could not be reopened");
assert(sprite.tags.length === 1, "tag was not persisted");
assert(sprite.tags[0].name === "Walk", "persisted tag name is wrong");
assert(sprite.tags[0].color === 0xFF3366CC, "persisted tag color is wrong");
assert(sprite.tags[0].aniDir === "pingpong", "persisted tag direction is wrong");

const persistedTag = sprite.tags[0];
sprite.removeTag(persistedTag);
assert(sprite.tags.length === 0, "tag was not removed");
console.log("frame tag bindings: PASS");
