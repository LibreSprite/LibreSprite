// LibreSprite
// Copyright (C) 2026 LibreSprite contributors

// Widget Gallery: a dialog that shows every widget the scripting API can
// create, wired to the event and storage APIs. Run it from the Scripts
// menu. Settings are saved to storage when you click Save and restored
// the next time the script runs.

const PW = 96, PH = 32; // preview image size

// The storage keys (the widget ids) whose values this dialog persists.
const KEYS = ["name", "count", "enabled", "level", "effect", "color", "items"];

// The dropdown and listbox options, in order. The widgets persist the
// selected item's TEXT, so the script keeps its own copy of the item
// texts to restore the selection.
const EFFECTS = ["None", "Smooth", "Sharp"];
const ITEMS = ["Alpha", "Beta", "Gamma"];

var dialog;
let name, count, enabled, level, effect, items, color, preview, palettes;

function renderPreview() {
    // Fill the preview with the color widget's color; the part left of
    // the slider level is full strength, the rest is darkened. Grayed
    // out while the checkbox is off.
    const data = new Uint8Array(PW * PH * 4);
    const c = color.color;
    let r = app.pixelColor.rgbaR(c);
    let g = app.pixelColor.rgbaG(c);
    let b = app.pixelColor.rgbaB(c);
    if (!enabled.checked)
        r = g = b = 40;
    const bar = Math.round(PW * level.value / level.max);
    for (let y = 0; y < PH; ++y) {
        for (let x = 0; x < PW; ++x) {
            const i = (y * PW + x) * 4;
            const f = (x < bar) ? 1 : 0.25;
            data[i] = (r * f) | 0;
            data[i + 1] = (g * f) | 0;
            data[i + 2] = (b * f) | 0;
            data[i + 3] = 255;
        }
    }
    preview.putImageData(data, PW, PH);
}

function saveAll() {
    for (const key of KEYS)
        storage.save(key);
    console.log("Widget Gallery: settings saved");
}

// Restore the persisted values (if any) into the widgets.
// storage.load() reads the file back as a string, so across sessions
// numbers come back like "75.000000" and booleans as "true"/"false";
// the values are coerced below (a same-session value keeps its type).
function restoreAll() {
    for (const key of KEYS)
        storage.load(key);

    if (storage.get("name") !== null)
        name.value = storage.get("name");
    if (storage.get("count") !== null)
        count.value = storage.get("count") | 0;
    const enabledVal = storage.get("enabled");
    if (enabledVal !== null)
        enabled.checked = (enabledVal === true) || (enabledVal === "true");
    if (storage.get("level") !== null)
        level.value = storage.get("level") | 0;
    // The dropdown persists the selected option's text.
    if (storage.get("effect") !== null)
        effect.value = storage.get("effect");
    // The listbox does the same; map the text back to an index with the
    // script's own copy of the items.
    const savedItem = storage.get("items");
    if (savedItem !== null) {
        const index = ITEMS.indexOf(savedItem);
        if (index >= 0)
            items.selectedIndex = index;
    }
    // The color persists as a 32-bit RGBA int.
    const colorVal = storage.get("color");
    if (colorVal !== null)
        color.color = parseInt(colorVal, 10) || 0;
}

function onEvent(...events) {
    // Events are delivered as individual arguments; the dialog resize
    // events carry numeric size arguments too, so skip non-strings.
    for (const e of events) {
        if (typeof e !== "string")
            continue;
        switch (e) {
            case "save_click":
                saveAll();
                break;
            case "close_click":
                dialog.close();
                break;
            default:
                // Any widget change re-renders the preview.
                if (e.endsWith("_change"))
                    renderPreview();
        }
    }
}

dialog = app.createDialog("gallery");
dialog.title = "Widget Gallery";

dialog.addLabel("Widget Gallery - every widget the script API can create", "title");
dialog.addBreak();

dialog.addEntry("Name:", "name");
dialog.addIntEntry("Count:", "count", 0, 99);
dialog.addBreak();

dialog.addCheckBox("Enabled:", "enabled");
dialog.addLabel("Level:");
dialog.addSlider("level", 0, 100);
dialog.addBreak();

dialog.addLabel("Effect:");
dialog.addDropDown("effect");
dialog.addLabel("Item:");
dialog.addListBox("items");
dialog.addBreak();

dialog.addColor("color", app.pixelColor.rgba(200, 60, 60, 255));
dialog.addButton("Save", "save");
dialog.addButton("Close", "close");
dialog.addBreak();

dialog.addLabel("Preview:");
dialog.addImageView("preview");
dialog.addBreak();

dialog.addTextBox("The preview follows the color, level and enabled\n" +
                  "widgets. Save writes the settings to storage; they\n" +
                  "are restored the next time this script runs.", "info");
dialog.addBreak();

dialog.addLabel("Palettes:");
dialog.addPaletteListBox("palettes");
dialog.addBreak();

dialog.addLinkLabel("LibreSprite scripting documentation",
                    "https://github.com/LibreSprite/LibreSprite/blob/master/SCRIPTING.md",
                    "docs");

// Keep local references (also usable via dialog.get("<id>")).
name = dialog.get("name");
count = dialog.get("count");
enabled = dialog.get("enabled");
level = dialog.get("level");
effect = dialog.get("effect");
items = dialog.get("items");
color = dialog.get("color");
preview = dialog.get("preview");
palettes = dialog.get("palettes");

// Populate the dropdown, listbox and palette listbox.
for (const text of EFFECTS)
    effect.addOption(text);
for (const text of ITEMS)
    items.add(text);
palettes.addPalette("Demo palette");
palettes.addPalette("Another palette");

restoreAll();
renderPreview();
