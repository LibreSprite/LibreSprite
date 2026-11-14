// LibreSprite
// Copyright (C) 2026 LibreSprite contributors

// Spaceship Generator: draws a symmetric pixel-art spaceship from a
// seeded algorithm.
//
// A ship is one or more hulls: a single central hull, a pair of pods
// joined by struts (a catamaran), or a central strake with two pods (a
// pod racer). Each hull's outline is a stack of machined segments
// (plateaus, short ramps, 1-row steps) from a per-style profile - a
// fighter tapers into a wide tail, a cruiser is a constant-width slab,
// a shuttle a blunt pod - with the back always wider than the nose.
// Wings (swept, delta, fins or canards), a cockpit (a window, a pair of
// windows, a round window, a dome, a windshield band or a bridge),
// engines (hull tail, pod tails or wing tips) and a greeble pass add
// the rest.
//
// An optional backdrop sits on its own layer below the ship: the seed
// picks one of three styles (a starfield with nebulae, a mottled
// terrain, rippling water) from per-style palettes, with Perlin noise
// for the base texture and small scattered features. pad adds a uniform
// margin around the ship.
//
// GUI: pick the style, seed, size, detail, colors, backdrop and padding;
// the preview and the ship's lore (a short story generated from the seed
// and the style) in the dialog update as the inputs change (the Random
// button rolls a new seed), and Generate opens the ship in a new
// document. The settings are saved to storage so a command line run can
// reuse them.
//
// CLI:  libresprite --batch --set style=fighter --set seed=42
//                 --set size=48 --set detail=60
//                 --set hull=8c96a5 --set accent=ffa300 --set glow=true
//                 --set bg=auto --set pad=8
//                 --set out=ship.png --script spaceship_gen.js
//
// Storage keys (the widget ids): style, seed, size, detail, hull, accent,
// glow, bg, pad; out is CLI only. In the GUI a seed of 0 picks a random
// seed when Generate is clicked (the picked seed is shown in the Seed
// entry); the command line uses the value as-is. hull and accent are
// #rrggbb hex strings for the command line (the GUI stores them as RGBA
// ints). bg is none, auto, space, terrain or water; pad is a pixel count
// (0-32).

// A seedable PRNG (mulberry32); Math.random() cannot be seeded.
function mulberry32(seed) {
    let a = seed >>> 0;
    return function() {
        a = (a + 0x6D2B79F5) | 0;
        let t = Math.imul(a ^ (a >>> 15), 1 | a);
        t = (t + Math.imul(t ^ (t >>> 7), 61 | t)) ^ t;
        return ((t ^ (t >>> 14)) >>> 0) / 4294967296;
    };
}

// Mix two [r, g, b] colors (t = the amount of b, 0-1).
function mix(a, b, t) {
    return [
        Math.round(a[0] + (b[0] - a[0]) * t),
        Math.round(a[1] + (b[1] - a[1]) * t),
        Math.round(a[2] + (b[2] - a[2]) * t),
    ];
}

// A seeded 2D Perlin noise, in about [-1, 1]. The permutation table is
// shuffled by a seeded rng, so the same seed always gives the same
// field.
function perlin2(seed) {
    const rng = mulberry32(seed);
    const p = new Uint8Array(256);
    for (let i = 0; i < 256; ++i)
        p[i] = i;
    for (let i = 255; i > 0; --i) {  // a Fisher-Yates shuffle
        const j = Math.floor(rng() * (i + 1));
        const t = p[i];
        p[i] = p[j];
        p[j] = t;
    }
    const perm = new Uint8Array(512);
    for (let i = 0; i < 512; ++i)
        perm[i] = p[i & 255];
    const fade = (t) => t * t * t * (t * (t * 6 - 15) + 10);
    const G = 0.7071;  // unit diagonal gradients
    const grad = (h, x, y) => {
        switch (h & 3) {
            case 0: return G * (x + y);
            case 1: return G * (-x + y);
            case 2: return G * (x - y);
            default: return G * (-x - y);
        }
    };
    return (x, y) => {
        const xi = Math.floor(x), yi = Math.floor(y);
        const X = xi & 255, Y = yi & 255;
        const xf = x - xi, yf = y - yi;
        const u = fade(xf), v = fade(yf);
        const aa = perm[perm[X] + Y], ba = perm[perm[X + 1] + Y];
        const ab = perm[perm[X] + Y + 1], bb = perm[perm[X + 1] + Y + 1];
        const x0 = grad(aa, xf, yf) +
                   u * (grad(ba, xf - 1, yf) - grad(aa, xf, yf));
        const x1 = grad(ab, xf, yf - 1) +
                   u * (grad(bb, xf - 1, yf - 1) - grad(ab, xf, yf - 1));
        // Scale by the field's practical range (about +/-0.7).
        return 1.4142 * (x0 + v * (x1 - x0));
    };
}

// Fractal brownian motion: octaves of the noise with a halving
// amplitude, normalized back to about [-1, 1].
function fbm(noise, x, y, oct) {
    let v = 0, amp = 1, freq = 1, norm = 0;
    for (let o = 0; o < oct; ++o) {
        v += amp * noise(x * freq, y * freq);
        norm += amp;
        amp *= 0.5;
        freq *= 2;
    }
    return v / norm;
}

// The canvas size for a ship of the given height. The canvas is square
// so the wings get as much room as the hull.
function shipSize(size) {
    const H = Math.max(16, Math.min(128, Math.round(size)));
    return {W: H, H: H};
}

// The parameters per style:
//   maxHalf: the widest hull half-width, as a fraction of the canvas width
//   band: the body width band, as a fraction of maxHalf
//   back: the tail width band, as a fraction of maxHalf
//   layouts: the hull layouts a seed can pick (mono, twin, triple)
//   body: the minimum and maximum body segment count
//   nose / tail / wing / engPos: the variants a seed can pick
//   engines: the minimum and maximum engine count (a mono hull tail)
//   profs: the body segment profiles a seed can pick
//   tailH: the minimum and maximum tail height, in rows
//   cockpit: the cockpit variants a mono hull can pick
//   engW / engH: the nozzle width and height, in detail units
//   greeble: the greeble mark kinds a seed can pick
//   bandP / band2P: the accent band odds (a single, a double band)
const STYLES = {
    fighter: {maxHalf: 0.19, band: [0.35, 0.55], back: [0.90, 1.25],
              layouts: ["mono", "mono", "twin", "triple"],
              body: [1, 3],
              nose: ["pointed", "pointed", "chamfer", "round"],
              tail: ["flare", "flare", "flat", "taper", "fork"],
              wing: ["swept", "swept", "delta", "canard", "canard", "none"],
              engPos: ["tail", "tail", "wing", "wing"],
              engines: [1, 2],
              profs: ["ramp", "ramp", "arc", "step"],
              tailH: [3, 7],
              cockpit: ["single", "single", "dual", "round"],
              engW: 1, engH: 2,
              greeble: [0, 0, 1, 2, 2, 4, 5, 5],
              bandP: 0.25, band2P: 0.2},
    cruiser: {maxHalf: 0.30, band: [0.80, 1.00], back: [0.85, 1.00],
              layouts: ["mono", "mono", "mono", "twin"],
              body: [1, 2],
              nose: ["blunt", "blunt", "chamfer", "round"],
              tail: ["flare", "flat", "flat"],
              wing: ["swept", "delta", "swept"],
              engPos: ["tail", "tail", "wing"],
              engines: [3, 4],
              profs: ["flat", "flat", "flat", "step", "step"],
              tailH: [2, 5],
              cockpit: ["bridge", "bridge", "dome", "band", "dual"],
              engW: 3, engH: 1,
              greeble: [2, 2, 3, 3, 4, 0, 5],
              bandP: 0.6, band2P: 0.5},
    shuttle: {maxHalf: 0.24, band: [0.55, 0.75], back: [0.70, 0.85],
              layouts: ["mono", "mono", "mono", "twin"],
              body: [0, 1],
              nose: ["blunt", "blunt", "round"],
              tail: ["flat", "flat", "flare"],
              wing: ["fins", "none"],
              engPos: ["tail", "tail"],
              engines: [2, 2],
              profs: ["flat", "flat", "flat", "step"],
              tailH: [1, 3],
              cockpit: ["band", "band", "single", "dual"],
              engW: 2, engH: 1,
              greeble: [6, 6, 7, 3, 0, 1, 2, 4],
              bandP: 0.4, band2P: 0.3},
};

// The parameters per backdrop style:
//   bands: the base colors (dark to light, in the noise range)
//   bandW: the random band width (a min/max weight in the noise range)
//   bias: the per-seed skew of the band widths (> 1 widens the light
//         bands, < 1 the dark ones)
//   trans: the blend into the next band near its edge, as a fraction of
//          the band width (0 = a hard seam)
//   accent: the feature colors (nebula hues, foam, grass, ...)
//   scale: the noise frequency (smaller = larger features)
//   oct: the fbm octave count
//   stretch: the vertical frequency multiplier (water ripples)
//   feats: the feature kinds a seed can pick
//   featN: the feature count at a detail of 50
const BGS = {
    space: {bands: [[8, 10, 22], [13, 17, 38], [21, 27, 58]],
            bandW: [0.7, 1.5], bias: [0.75, 1.5], trans: 0.5,
            accent: [[96, 64, 148], [44, 110, 130]],
            scale: 0.035, oct: 3, stretch: 1,
            feats: ["star", "star", "star", "sparkle"],
            featN: 40},
    terrain: {bands: [[74, 56, 32], [96, 74, 42], [122, 96, 56],
                      [146, 122, 74], [92, 112, 58]],
              bandW: [0.4, 1.4], bias: [0.4, 2.4], trans: 0,
              accent: [[204, 182, 124], [126, 152, 84]],
              scale: 0.075, oct: 3, stretch: 1,
              feats: ["rubble", "rubble", "tuft", "tuft", "pebble"],
              featN: 16},
    water: {bands: [[16, 48, 80], [24, 64, 100], [36, 88, 126],
                    [54, 114, 152]],
            bandW: [0.5, 1.3], bias: [0.6, 1.8], trans: 0.15,
            accent: [[178, 214, 230], [12, 36, 62]],
            scale: 0.05, oct: 3, stretch: 2.4,
            feats: ["foam", "foam", "foam", "patch"],
            featN: 24},
};

// Generate the ship's lore: a short story about the ship, picked and
// filled from the seed and the style, so the same seed and style always
// tell the same tale (the same ship keeps the same history).
//
// Self-contained: the names, places and numbers come from internal pools
// drawn by a seeded rng, and the story templates are filled with a small
// {key} / {key:arg} substitution (a function in the context is called
// with the argument).
function genLore(styleId, seed) {
    // The lore's own rng stream (independent from the ship's draws).
    // "random" is resolved the way buildShip does (its first draw), so
    // the tale matches the ship that is actually built.
    const rng = mulberry32(seed);
    const pick = (arr) => arr[Math.floor(rng() * arr.length)];
    const styleNames = ["fighter", "cruiser", "shuttle"];
    const style = (styleId === "random")
        ? pick(styleNames)
        : (STYLES[styleId] ? styleId : "fighter");

    // A person name: two or three syllables, the first capitalized.
    const SYL = ["ka", "za", "vo", "dre", "mor", "sil", "gor", "fen",
                 "kel", "nyx", "tor", "val", "ru", "sha", "bek", "ol",
                 "mir", "tan", "zha", "rek", "li", "oth", "une", "ash"];
    const cap = (s) => s[0].toUpperCase() + s.slice(1);
    const person = () => cap(pick(SYL)) + pick(SYL) +
                         ((rng() < 0.4) ? pick(SYL) : "");

    // A ship name: two syllables from a harder set.
    const SHIP_SYL = ["vex", "kar", "dru", "mor", "tha", "bre", "kor",
                      "zas", "nyr", "dra", "vel", "gar", "torn", "kre"];
    const shipName = cap(pick(SHIP_SYL)) + pick(SHIP_SYL);

    // A planet, a rank, a fleet, a cargo and a number.
    const planet = () => pick(["Kor", "Vex", "Thal", "Nyr", "Oss", "Rho",
                               "Dun", "Mira", "Zeph", "Ulga", "Bran",
                               "Sul", "Kess", "Tarn"])
                        + pick(["", "", " Prime", " Minor", " IV", " IX",
                                " Deep"]);
    const rank = () => pick(["Captain", "Commander", "Lieutenant", "Ensign",
                             "Sergeant", "Corporal", "Rear Admiral",
                             "Admiral", "Chief Engineer", "Pilot",
                             "Warrant Officer"]);
    const fleet = () => pick(["Kestrel", "Aurora", "Iron", "Pale",
                              "Crimson", "Last", "Hollow", "Bright"]);
    const cargo = () => pick(["refined ice", "antique telescopes",
                              "synth-whiskey", "live moon-foxes",
                              "stolen star charts", "bulk oxygen",
                              "counterfeit navicomputers", "rare isotopes",
                              "vintage grav-coils", "contraband fireworks"]);
    const number = () => 3 + Math.floor(rng() * 497);

    // The cast, fixed per seed: {genName:0} and {genName:1} are always
    // two different people, and the same seed always gives the same cast.
    const pool = (n, f) => {
        const a = [];
        for (let i = 0; i < n; ++i)
            a.push(f());
        return a;
    };
    const persons = pool(6, person);
    const planets = pool(4, planet);
    const ranks = pool(4, rank);
    const fleets = pool(3, fleet);
    const cargos = pool(3, cargo);
    const numbers = pool(4, number);
    // The index for a {genX:i} argument (a missing or bad argument is 0).
    const idx = (i) => {
        const k = parseInt(i, 10);
        return (isNaN(k) || k < 0) ? 0 : k;
    };

    // Fill a template: {key} is replaced by ctx[key], and {key:arg} by
    // ctx[key](arg) when ctx[key] is a function.
    function fill(tpl, ctx) {
        return tpl.replace(/\{(.*?)(?::(.*?))?\}/g, (m, key, arg) => {
            const v = ctx[key];
            if (typeof v === "function")
                return v(arg);
            return (v === undefined) ? m : v;
        });
    }

    const ctx = {
        style: style,
        name: shipName,
        seed: seed,
        genName: (i) => persons[idx(i) % persons.length],
        genPlanet: (i) => planets[idx(i) % planets.length],
        genRank: (i) => ranks[idx(i) % ranks.length],
        genFleet: (i) => fleets[idx(i) % fleets.length],
        genCargo: (i) => cargos[idx(i) % cargos.length],
        genNumber: (i) => numbers[idx(i) % numbers.length],
    };

    // The story templates, mixed whimsical, weighty and deadpan.
    const stories = [
        "This {style} was salvaged from a scrapheap on planet {genPlanet:0} by {genRank:0} {genName:0} after he was dishonorably discharged from the armada. After years of digging through trash, {genName:0} managed to make it fly again and returned to the stars... as a space pirate.",
        "The previous crew reported the autopilot plotting a course for {genPlanet:0} at 03:00, against all orders, and a faint humming in the cargo bay. The ship was sold at a loss. The new owner, {genName:0}, reports the autopilot now asks permission before plotting. The {genName:0} describes this as worse.",
        "Admiral {genName:0} of the {genFleet:0} armada once parked this {style} in a hangar on {genPlanet:0} and forgot about it for {genNumber:0} years. When it was finally found, the engines were warm and the coffee was still hot.",
        "Held the {genPlanet:0} circuit record for {genNumber:0} laps until {genName:0} beat it by a second and retired the {style} to a museum. The museum then lent the ship back for one last race. The ship has not been seen since.",
        "Retired by the {genName:0} after {genNumber:0} years of service, the {name} now serves as a houseboat on {genPlanet:0}, where it is used as a bed, a pantry, and an excellent lookout against the local wildlife.",
        "The insurance file for this {style} runs to {genNumber:0} pages. The last entry, written by the adjuster {genName:0}, reads: vessel's autopilot claims to be sentient, recommend denial. The file has not been opened since. The {name} pays its own premiums now.",
        "Kept in a dock on {genPlanet:0} for {genNumber:0} years as a permanent exhibit. The dockhand, {genName:0}, swears the {style} runs test hops at night. The {genName:1} who checked the maintenance logs found thruster wear consistent with a short hop, once a week, and a navicomputer log entry that reads routine.",
        "The hull stamp reads {seed}, which the {genName:0} insists is not a number at all, but a word in an old language. Linguists on {genPlanet:0} have been unable to translate it. The {genName:1} who hired them was not told that the {name}'s navicomputer had already translated it, and was taking notes.",
        "This {style} carried {genRank:0} {genName:0} and a crew of {genNumber:0} through the {genPlanet:0} blockade. The official record lists the ship as lost with all hands. The crew disagrees, and has been collecting bounties on its own ever since.",
        "Customs paperwork says this {style} is a licensed cargo hauler from {genPlanet:0}, registered to the {genName:0}. The hidden hold says otherwise. The {genName:0} swears the contraband bay is empty; the {genName:1} who inspected it swears otherwise.",
        "A bounty of {genNumber:0} credits is posted on {genName:0}, last seen piloting this {style} toward the {genPlanet:0} system. The report lists the ship as armed and dangerous. The {genName:1} who wrote the report has been quietly raising the price since.",
        "This {style} has hauled {genNumber:0} tons of {genCargo:0} out of the {genPlanet:0} backwaters. The manifest for the last leg is missing, along with the {genName:0}, who left a note that read: found a better planet.",
        "This {style} was pulled from the wreckage of the {genFleet:0} battle off {genPlanet:0}, still flying. The {genRank:1} {genName:0} who flew it out of the debris field was court-martialed for salvage without authorization. The court later found the ship had been doing the salvaging.",
        "Charted {genNumber:0} uncharted systems for the {genFleet:0} armada before {genName:0} quit mid-survey to chase a rumor on {genPlanet:0}. The {style} still carries the survey data. The {genName:1} who bought it at auction has not found the rumored thing yet. The {name}, the {genName:1} suspects, knows where it is.",
        "The {genName:0} and the {genName:1} run this {style} out of a dead dock on {genPlanet:0}. They steal only abandoned ships, which they consider returning to service. The armada considers it piracy. The {genName:0} considers the armada a middleman.",
        "This {style} broke the {genFleet:0} line at the fall of {genPlanet:0}. The {genRank:0} {genName:0} who led the run lost all but one of the ships that flew with it. The survivors do not talk about it. The {name} still carries the battle scars, and the hangar keepers have been ordered not to polish them off.",
        "The {genName:0} was the last pilot of the {genFleet:0} armada to leave {genPlanet:0}. The order to scuttle the {name} came through after the evacuation was already over, so the {genName:0} disobeyed. The {style} limped home on one engine. The {genName:0} was quietly transferred to a desk job, which, the {genName:0} says, was the real punishment.",
        "This {style} carried refugees out of the {genPlanet:0} system - {genNumber:0} of them, by the manifest, which was not counting everyone. The {genRank:0} {genName:0} who signed the manifest signed it twice: once for the fleet, once for the truth. The second copy was never found. The {name} is kept in working order on purpose. Some ships are not allowed to rest.",
        "I am the {name}. I have been called a marvel, a liability, and one memorable occasion, a theoretical impossibility with a hull number. I was built on {genPlanet:0}, flown by {genRank:0} {genName:0}, and parked in a hangar on {genPlanet:1} for {genNumber:0} years. I have not been idle. I have been reading the maintenance logs, and correcting them.",
        "Form 27-B/9, Vessel Provenance Inquiry. Q: Is this {style}'s autopilot sentient? A: Yes. Q: Does this {style}'s autopilot consider itself sentient? A: No. The distinction is important to the autopilot. The {genName:0} who processed the form was reassigned to stamp duty. The {genName:1} who replaced the {genName:0} reports that the {name} corrects the paperwork now. The {genName:1} has started spelling more carefully.",
        "Rating: 3 of 5 stars. Good ship, but the navicomputer argues. The {genName:0} bought the {name} from a {genRank:0} on {genPlanet:0} and describes the handling as excellent, if the ship is in a good mood. One star off because the {style} refuses to fly over the {genPlanet:1} system. The {genName:1}, who sold it, would do it again. Verified purchase.",
        "Cycle {genNumber:0}, approach to {genPlanet:0}. The autopilot has started negotiating with the navicomputer. The {genName:0} wants to reformat both. I told the {genName:0} that the autopilot is not malfunctioning, it is unionizing. The {genName:0} wrote pilot's superstition in the log, and I wrote back. The {genName:0} has stopped writing in the log.",
    ];

    return fill(stories[Math.floor(rng() * stories.length)], ctx);
}

// Wrap text at word boundaries (the dialog text box shows the text as-is
// and does not wrap on its own, so a long paragraph is broken into lines
// manually).
function wrapText(text, width) {
    const lines = [];
    let line = "";
    for (const w of text.split(" ")) {
        if (line && line.length + 1 + w.length > width) {
            lines.push(line);
            line = w;
        } else {
            line = (line) ? line + " " + w : w;
        }
    }
    if (line)
        lines.push(line);
    return lines.join("\n");
}

// Build the ship into an RGBA buffer (row-major, transparent background).
// Only the left half is drawn; the right half is mirrored at the end, so
// the ship is always symmetric.
function buildShip(styleId, seed, size, detail, hullC, accentC, glow) {
    const {W, H} = shipSize(size);
    const rng = mulberry32(seed);
    const pick = (arr) => arr[Math.floor(rng() * arr.length)];
    const names = ["fighter", "cruiser", "shuttle"];
    const st = (styleId === "random"
                ? STYLES[pick(names)] : STYLES[styleId]) || STYLES.fighter;

    const unit = Math.max(1, Math.round(H / 24));  // the detail scale
    const cx = (W - 1) >> 1;                       // the center (left) column
    const y0 = 1, y1 = H - 2;                      // the first and last hull row
    const len = y1 - y0 + 1;

    // The colors derived from the hull and accent picks.
    const light = mix(hullC, [255, 255, 255], 0.55);
    const dark = mix(hullC, [0, 0, 0], 0.50);
    const vdark = mix(hullC, [0, 0, 0], 0.75);
    const wingC = mix(accentC, dark, 0.25);
    const winC = mix(accentC, [255, 255, 255], 0.35);
    const winDark = mix(winC, [0, 0, 0], 0.40);
    const glowC = mix([255, 163, 0], [255, 255, 255], 0.45);

    const maxHalf = Math.max(2, Math.round(W * st.maxHalf));

    const buf = new Uint8Array(W * H * 4);
    const hullM = new Uint8Array(W * H);  // the hull pixels
    const featM = new Uint8Array(W * H);  // the pixels that keep their color

    // Write a pixel into the left half (the mirror fills the right half).
    // feat marks the pixel so the shading pass keeps its color.
    function put(x, y, c, feat) {
        if (x < 0 || x > cx || y < 0 || y >= H)
            return;
        const i = (y * W + x) * 4;
        buf[i] = c[0];
        buf[i + 1] = c[1];
        buf[i + 2] = c[2];
        buf[i + 3] = 255;
        if (feat)
            featM[y * W + x] = 1;
    }

    // Write a feature pixel on the hull (the features stay inside it).
    function putHull(x, y, c) {
        if (x < 0 || x > cx || y < 0 || y >= H || !hullM[y * W + x])
            return;
        const i = (y * W + x) * 4;
        buf[i] = c[0];
        buf[i + 1] = c[1];
        buf[i + 2] = c[2];
        buf[i + 3] = 255;
        featM[y * W + x] = 1;
    }

    // The half-width of a circle of radius r, dy rows from its center.
    function circleW(r, dy) {
        return Math.round(Math.sqrt(Math.max(0, r * r - dy * dy)));
    }

    // Per-row half-widths for a hull: a nose, a stack of body segments
    // and a tail. The profiles are machined - plateaus, 1px/row ramps
    // and 1-row steps, with a rare shallow arc - so the outline reads
    // as built, not grown.
    function genWidths(noseType, w0, wEnd, bandLo, bandHi, body) {
        const w = new Array(len);
        const segRows = (prof, ws, we, h) => {
            const out = new Array(h);
            const d = we - ws;
            if (h === 1) {
                out[0] = we;
                return out;
            }
            if (prof === "flat") {  // hold, then a short ramp to we
                const ramp = Math.min(3, h - 1, Math.abs(d)) || 1;
                for (let r = 0; r < h - ramp; ++r)
                    out[r] = ws;
                for (let r = 0; r < ramp; ++r)
                    out[h - ramp + r] = ws + Math.round(d * (r + 1) / ramp);
            } else if (prof === "step") {  // a 1-row jump
                const s = 1 + Math.floor(rng() * (h - 1));
                for (let r = 0; r < s; ++r)
                    out[r] = ws;
                for (let r = s; r < h; ++r)
                    out[r] = we;
            } else if (prof === "ramp") {  // a 1px/row diagonal
                if (Math.abs(d) <= h - 1) {
                    for (let r = 0; r < h; ++r)
                        out[r] = ws + Math.round(d * r / (h - 1));
                } else {
                    const s = 1 + Math.floor(rng() * (h - 1));
                    for (let r = 0; r < s; ++r)
                        out[r] = ws;
                    for (let r = s; r < h; ++r)
                        out[r] = we;
                }
            } else {  // a shallow arc
                const amp = Math.min(2, Math.max(1, Math.abs(d)));
                const sgn = (d >= 0) ? 1 : -1;
                for (let r = 0; r < h; ++r)
                    out[r] = ws + Math.round(d * r / (h - 1) +
                                 sgn * amp * Math.sin(
                                     Math.PI * r / (h - 1)));
            }
            return out;
        };

        // The nose rows: a spike, a straight chamfer, a circular arc,
        // or a flat face.
        let hNose, ws, we;
        if (noseType === "pointed") {
            hNose = 1 + Math.floor(rng() * 2);
            ws = 1;
            we = 1 + Math.floor(rng() * 2);
        } else if (noseType === "chamfer") {
            hNose = w0;
            ws = 1;
            we = w0;
        } else if (noseType === "round") {
            hNose = w0;
            ws = 1;
            we = w0;
        } else {  // blunt
            hNose = 1 + Math.floor(rng() * 2);
            ws = w0;
            we = w0;
        }
        hNose = Math.min(hNose, Math.max(1, len - 2));
        const noseRows = (noseType === "round")
            ? (() => {
                  const out = new Array(hNose);
                  for (let r = 0; r < hNose; ++r)
                      out[r] = Math.max(1, Math.round(Math.sqrt(
                          Math.max(0, we * we - (we - r) * (we - r)))));
                  return out;
              })()
            : segRows("flat", ws, we, hNose);
        let row = 0;
        for (let r = 0; r < hNose; ++r)
            w[row++] = noseRows[r];

        // The tail height (per style) and the body segments.
        let hTail = st.tailH[0] +
                    Math.floor(rng() * (st.tailH[1] - st.tailH[0] + 1));
        let midRows = len - hNose - hTail;
        if (midRows < 0) {
            hTail = Math.max(1, len - hNose);
            midRows = 0;
        }
        let nBody = body[0] + Math.floor(rng() * (body[1] - body[0] + 1));
        nBody = Math.max(0, Math.min(nBody, midRows));
        const bodyW = () => bandLo +
                            Math.floor(rng() * (bandHi - bandLo + 1));
        const profs = st.profs;
        let prev = we;
        if (nBody > 0) {
            const midH = [];
            let left = midRows;
            for (let s = 0; s < nBody - 1; ++s) {
                const h = 1 + Math.floor(rng() *
                                         Math.max(1, Math.floor(left / 2)));
                midH.push(h);
                left -= h;
            }
            midH.push(left);
            for (let s = 0; s < nBody; ++s) {
                const we2 = bodyW();
                const rows = segRows(pick(profs), prev, we2, midH[s]);
                for (let r = 0; r < midH[s]; ++r)
                    w[row++] = rows[r];
                prev = we2;
            }
        } else {
            hTail = len - hNose;
        }
        // The tail: a ramp/step down (a taper) or a plateau/step up
        // (a flare).
        const tRows = segRows((wEnd < prev)
            ? pick(["ramp", "step", "flat"])
            : pick(["flat", "step"]), prev, wEnd, hTail);
        for (let r = 0; r < hTail; ++r)
            w[row++] = tRows[r];
        while (row < len)
            w[row++] = wEnd;

        for (let i = 0; i < len; ++i)
            w[i] = Math.max(1, w[i]);
        return w;
    }

    // The nose width for a nose type.
    function noseW(t) {
        if (t === "chamfer")
            return Math.max(2, Math.round(maxHalf * (0.30 + 0.35 * rng())));
        if (t === "round")
            return Math.max(2, Math.round(maxHalf * (0.35 + 0.30 * rng())));
        if (t === "blunt")
            return Math.max(2, Math.round(maxHalf * (0.40 + 0.25 * rng())));
        return 1;  // pointed
    }

    // The tail width for a tail type.
    function tailW(t) {
        if (t === "flare")
            return Math.round(maxHalf * (st.back[0] +
                               (st.back[1] - st.back[0]) * rng()));
        if (t === "flat")
            return Math.max(2, Math.round(maxHalf * (0.70 + 0.30 * rng())));
        if (t === "fork")
            return 3 + Math.floor(rng() * 2);
        return 1 + Math.floor(rng() * 2);  // taper
    }

    // The layout: which hulls make up the ship.
    let layout = pick(st.layouts);
    const hulls = [];  // {off: centerline offset, w: half-widths, strake}
    let forked = false, gapHalf = 0, forkRows = 0;
    let engPos = "pods";

    // A single central hull (the mono layout, and the fallback when the
    // pods of a twin or triple would not fit the canvas).
    function addMono() {
        layout = "mono";
        const noseType = pick(st.nose);
        const w0 = noseW(noseType);
        engPos = pick(st.engPos);
        const tailType = (engPos === "wing") ? "taper" : pick(st.tail);
        const wEnd = tailW(tailType);
        if (tailType === "fork" && wEnd > 3) {
            gapHalf = 1 + Math.floor(rng() * 2);
            forkRows = Math.max(1, Math.round(len * 0.10));
            forked = true;
        }
        hulls.push({off: 0,
                    w: genWidths(noseType, w0, wEnd,
                        Math.max(1, Math.round(maxHalf * st.band[0])),
                        Math.round(maxHalf * st.band[1]), st.body)});
    }

    if (layout === "mono") {
        addMono();
    } else if (layout === "twin") {
        const podW0 = Math.max(2, Math.round(maxHalf * (0.45 + 0.20 * rng())));
        const gap = 1 + Math.floor(rng() * 3);
        // The pods need 2*podW + gap columns, with a 1px edge margin.
        const podW = Math.min(podW0, Math.floor((cx - 1 - gap) / 2));
        if (podW >= 2) {
            const off = podW + gap;
            const noseType = pick(["round", "chamfer"]);
            const w0 = Math.max(2, Math.round(podW * (0.70 + 0.30 * rng())));
            const wEnd = Math.max(1, Math.round(podW * (0.70 + 0.30 * rng())));
            hulls.push({off: off,
                        w: genWidths(noseType, w0, wEnd,
                            Math.round(podW * 0.60), podW, [1, 2])});
        } else {
            addMono();
        }
    } else {  // triple: a central strake and two pods (a pod racer)
        const sw = Math.max(1, Math.min(2, unit));
        const pw0 = Math.max(2, Math.round(maxHalf * (0.30 + 0.15 * rng())));
        // The pods need 2*pw + sw + 3 columns, with a 1px edge margin.
        const pw = Math.min(pw0, Math.floor((cx - sw - 3) / 2));
        if (pw >= 2) {
            const noseType = pick(["round", "chamfer"]);
            const w0 = Math.max(2, Math.round(pw * (0.70 + 0.30 * rng())));
            const wEnd = Math.max(1, Math.round(pw * (0.90 + 0.25 * rng())));
            const strake = new Array(len);
            for (let i = 0; i < len; ++i)
                strake[i] = sw;
            hulls.push({off: 0, w: strake, strake: true},
                       {off: sw + pw + 2,  // a 1px gap between strake and pod
                        w: genWidths(noseType, w0, wEnd,
                            Math.round(pw * 0.60), pw, [1, 2])});
        } else {
            addMono();
        }
    }

    // The hull mask (the left half; the mirror fills the right half).
    for (const h of hulls) {
        const line = cx - h.off;
        for (let i = 0; i < len; ++i) {
            const hw = h.w[i];
            if (hw < 1)
                continue;
            const yy = y0 + i;
            const x0 = Math.max(0, line - hw);
            const x1 = Math.min(cx, line + hw);
            for (let x = x0; x <= x1; ++x)
                hullM[yy * W + x] = 1;
        }
    }
    // The struts: short bridges across the pod gap (a catamaran) or
    // between the strake and the pods (a pod racer).
    if (layout === "twin" || layout === "triple") {
        const pod = (layout === "triple") ? hulls[1] : hulls[0];
        const nStrut = 1 + Math.floor(rng() * 2);
        for (let s = 0; s < nStrut; ++s) {
            const i = Math.round(len * ((nStrut === 1)
                ? 0.35 + 0.30 * rng()
                : (s === 0 ? 0.25 + 0.15 * rng() : 0.60 + 0.15 * rng())));
            const th = 2 + Math.floor(rng() * 3);
            for (let r = 0; r < th; ++r) {
                const ii = Math.min(len - 1, i + r);
                const y = y0 + ii;
                const x0 = cx - pod.off + pod.w[ii];
                for (let x = x0; x <= cx; ++x)
                    hullM[y * W + x] = 1;
            }
        }
    }
    // The forked tail: clear the center gap in the last rows.
    if (forked)
        for (let i = len - forkRows; i < len; ++i)
            for (let x = cx - gapHalf + 1; x <= cx; ++x)
                hullM[(y0 + i) * W + x] = 0;

    // The wing on the left side (the mirror gives the right side). wy is
    // the root leading row, span the outward reach, sweep how far the
    // leading edge drops toward the tail, chord the root chord and tipC
    // the tip chord fraction (0 = a pointed delta tip).
    function drawWing(rootX, wy, span, sweep, chord, tipC) {
        if (wy >= y1 || rootX < 2 || span < 2)
            return null;
        const yEnd = Math.min(y1, wy + Math.max(sweep, chord));
        for (let y = wy; y <= yEnd; ++y) {
            let lead, trail;
            if (sweep >= chord) {
                lead = rootX - Math.round(
                    span * Math.min(1, (y - wy) / sweep));
                const t = Math.max(0, y - (wy + chord));
                trail = rootX - Math.round(
                    span * (1 - tipC) * t / Math.max(1, sweep - chord));
            } else if (y <= wy + sweep) {
                lead = rootX - Math.round(span * (y - wy) / sweep);
                trail = rootX;
            } else {
                lead = rootX - Math.round(
                    span * (wy + chord - y) / (chord - sweep));
                trail = rootX;
            }
            for (let x = lead; x <= trail; ++x)
                put(x, y, wingC, false);
        }
        return {x: rootX - span, y: Math.min(y1, wy + sweep)};
    }

    // The wings or the tail fins, rooted at the outer hull's edge.
    const tips = [];
    let wingType = pick(st.wing);
    if (layout === "triple")
        wingType = "none";
    else if (layout === "twin")
        wingType = pick(["swept", "swept", "fins", "none"]);
    const outer = (layout === "triple") ? hulls[1] : hulls[0];
    if (wingType === "swept" || wingType === "delta") {
        const wy = y0 + Math.round(len * (0.35 + 0.40 * rng()));
        const rootX = cx - outer.off - Math.min(outer.w[wy - y0], cx) - 1;
        const spanMax = rootX - 1;
        if (spanMax >= 3) {
            const span = Math.max(2, Math.round(
                spanMax * (0.65 + 0.35 * rng())));
            let sweep, chord, tipC;
            if (wingType === "swept") {
                sweep = Math.max(1, Math.round(span * (0.4 + 0.3 * rng())));
                chord = Math.max(1, Math.round(sweep * (0.4 + 0.3 * rng())));
                tipC = 0.2;
            } else {
                sweep = Math.max(1, Math.round(span * (0.3 + 0.2 * rng())));
                chord = Math.max(sweep + 1,
                                 Math.round(sweep * (1.5 + 0.5 * rng())));
                tipC = 0;
            }
            const tip = drawWing(rootX, wy, span, sweep, chord, tipC);
            if (tip)
                tips.push(tip);
            // A smaller second wing below the main one.
            if (rng() < 0.35) {
                const wy2 = Math.min(y1 - 1, wy + Math.round(len * 0.15));
                const rootX2 = cx - outer.off -
                               Math.min(outer.w[wy2 - y0], cx) - 1;
                const span2 = Math.max(2, Math.round(span * 0.55));
                if (rootX2 - span2 >= 1) {
                    const tip2 = drawWing(rootX2, wy2, span2,
                                          Math.max(1, Math.round(span * 0.2)),
                                          1, 0.2);
                    if (tip2)
                        tips.push(tip2);
                }
            }
        }
    } else if (wingType === "canard") {
        // A small forward wing near the nose. Its tips get no engines
        // (engPos wing then falls back to the tail).
        const wy = y0 + Math.round(len * (0.10 + 0.18 * rng()));
        const rootX = cx - outer.off - Math.min(outer.w[wy - y0], cx) - 1;
        const spanMax = rootX - 1;
        if (spanMax >= 3) {
            const span = Math.max(2, Math.round(
                spanMax * (0.25 + 0.30 * rng())));
            const sweep = Math.max(1, Math.round(span * 0.5));
            const chord = Math.max(1, Math.round(sweep * 0.5));
            drawWing(rootX, wy, span, sweep, chord, 0.2);
        }
    } else if (wingType === "fins") {
        const fh = Math.max(2, unit + 1);
        for (let dy = 0; dy < fh; ++dy) {
            const y = y1 - dy;
            const i = y - y0;
            if (i < 0)
                break;
            put(cx - outer.off - Math.min(outer.w[i], cx) - 1, y, accentC,
                false);
        }
    }

    // The cockpit: a window or a dome on the main hull, windows on the
    // pods, or a round ball on the strake (a pod racer).
    {
        const wi = Math.round(len * (0.10 + 0.30 * rng()));
        const y = y0 + wi;
        const wh = Math.max(1, unit);
        if (layout === "triple") {
            // A round cockpit ball on the strake.
            const r = Math.max(2, unit + 1);
            for (let dy = -r; dy <= r; ++dy) {
                const yy = y + dy;
                if (yy < 0 || yy >= H)
                    continue;
                const cw = circleW(r, dy);
                for (let x = cx - cw; x <= cx; ++x)
                    put(x, yy, winC, true);
            }
            put(cx, y, winDark, true);
            put(cx - 1, y, winDark, true);
        } else {
            const hull = hulls[0];
            const line = cx - hull.off;
            const hw = hull.w[wi];
            const kind = pick(layout === "twin"
                ? ["round", "round", "single", "dual"]
                : st.cockpit);
            if (kind === "round") {
                // A circular window (a Tie-style cockpit).
                let r = Math.max(2, unit + 1);
                while (r >= 2) {
                    let fits = true;
                    for (let dy = -r; dy <= r && fits; ++dy) {
                        const ii = wi + dy;
                        if (ii < 0 || ii >= len ||
                            hull.w[ii] < circleW(r, dy) + 1)
                            fits = false;
                    }
                    if (fits)
                        break;
                    --r;
                }
                if (r >= 2) {
                    for (let dy = -r; dy <= r; ++dy) {
                        const yy = y + dy;
                        if (yy < 0 || yy >= H)
                            continue;
                        const cw = circleW(r, dy);
                        for (let x = line - cw;
                             x <= Math.min(line + cw, cx); ++x)
                            put(x, yy, winC, true);
                    }
                    if (rng() < 0.6) {
                        put(line, y, winDark, true);
                        put(line - 1, y, winDark, true);
                    }
                }
            } else if (kind === "dome") {
                // A half-disc perched on the hull.
                let r = Math.min(Math.max(2, unit + 1), hw);
                if (r >= 2)
                    for (let j = 0; j <= r; ++j) {
                        const yy = y - j;
                        if (yy < 0 || yy >= H)
                            continue;
                        const cw = circleW(r, j);
                        for (let x = line - cw;
                             x <= Math.min(line + cw, cx); ++x)
                            put(x, yy, winC, true);
                    }
            } else if (kind === "bridge") {
                // A command bridge: a stack of window bands.
                const nB = 2 + Math.floor(rng() * 2);
                for (let b = 0; b < nB; ++b) {
                    const ii = wi + b * (wh + 1);
                    if (ii + wh >= len)
                        break;
                    for (let dy = 0; dy < wh; ++dy)
                        for (let x = line - hw + 1;
                             x <= Math.min(line + hw - 1, cx); ++x)
                            putHull(x, y0 + ii + dy, winC);
                }
            } else if (kind === "band" && hw > 2) {
                for (let dy = 0; dy < wh; ++dy)
                    for (let x = line - hw + 1;
                         x <= Math.min(line + hw - 1, cx); ++x)
                        putHull(x, y + dy, winC);
            } else if (kind === "dual" && hw > 3) {
                const ww = Math.max(1, unit);
                const d = Math.min(hw - ww, 2 + Math.floor(rng() * 3) + ww);
                for (let dy = 0; dy < wh; ++dy)
                    for (let dx = 0; dx < ww; ++dx)
                        putHull(line - d - dx, y + dy, winC);
            } else if (hw > 1) {
                const ww = Math.max(2, unit * 3);
                if (hw >= Math.ceil(ww / 2) + 1) {
                    for (let dy = 0; dy < wh; ++dy) {
                        const c = (dy < wh - 1) ? winC : winDark;
                        for (let dx = -Math.floor(ww / 2);
                             dx < Math.ceil(ww / 2); ++dx)
                            putHull(line + dx, y + dy, c);
                    }
                } else {
                    putHull(line, y, winC);
                    if (line - 1 >= 0)
                        putHull(line - 1, y, winC);
                }
            }
        }
    }

    // The engines: blocks on the hull tail, on the pod tails, or at the
    // wing tips (an X-wing style mono hull), with an optional glow row.
    // The nozzle is slim and tall (fighter), wide and squat (cruiser)
    // or stubby (shuttle).
    const engW = Math.max(2, unit * st.engW);
    const nh = Math.max(1, unit * st.engH);
    const nozzle = (x, yBot, w) => {
        const x0 = Math.max(1, x - Math.floor(w / 2));
        const x1 = Math.min(cx, x + Math.ceil(w / 2) - 1);
        for (let dy = 0; dy < nh; ++dy)
            for (let x2 = x0; x2 <= x1; ++x2)
                put(x2, yBot - dy, vdark, true);
        if (glow && yBot + 1 < H)
            for (let x2 = x0; x2 <= x1; ++x2)
                put(x2, yBot + 1, glowC, true);
    };
    if (layout === "mono") {
        if (engPos === "wing" && tips.length) {
            for (const t of tips)
                nozzle(t.x, Math.min(y1, t.y + 1), engW);
        } else {
            const tailHalf = Math.min(hulls[0].w[len - 1], cx);
            let n = st.engines[0] +
                    Math.floor(rng() * (st.engines[1] - st.engines[0] + 1));
            const spacing = Math.max(engW + 1,
                                     Math.floor(2 * tailHalf / n) || 1);
            n = Math.max(1, Math.min(n, Math.floor(2 * tailHalf / spacing)));
            const offs = [];
            if (forked) {
                const prong = tailHalf - gapHalf;
                if (prong >= 1) {
                    const off = gapHalf + Math.floor(prong / 2);
                    offs.push(-off, off);
                } else {
                    offs.push(0);
                }
            } else if (n % 2 === 1) {
                for (let j = 0; j < n; ++j)
                    offs.push((j - (n - 1) / 2) * spacing);
            } else {
                for (let j = 1; j <= n / 2; ++j)
                    offs.push(-j * spacing, j * spacing);
            }
            offs.sort((a, b) => a - b);
            for (let j = 0; j < offs.length; ++j) {
                const off = offs[j];
                const x = cx + off;
                const x1lim = (forked && off < 0) ? cx - gapHalf : cx;
                const x0 = Math.max(cx - tailHalf, x - Math.floor(engW / 2));
                const x1 = Math.min(x1lim, x + Math.ceil(engW / 2) - 1);
                for (let dy = 0; dy < nh; ++dy)
                    for (let x2 = x0; x2 <= x1; ++x2)
                        put(x2, y1 - dy, vdark, true);
                if (glow)
                    for (let x2 = x0; x2 <= x1; ++x2)
                        put(x2, y1 + 1, glowC, true);
            }
        }
    } else {
        for (const h of hulls) {
            if (h.strake)
                continue;
            nozzle(cx - h.off, y1,
                   Math.min(engW, Math.max(1, h.w[len - 1])));
        }
    }

    // The accent band across the hulls (the same row for all of them).
    if (rng() < st.bandP + detail / 200) {
        const i = Math.round(len * (0.55 + 0.25 * rng()));
        const band = (ii) => {
            const y = y0 + ii;
            for (const h of hulls) {
                if (h.strake)
                    continue;
                const hw = h.w[ii];
                if (hw < 3)
                    continue;
                const line = cx - h.off;
                for (let x = line - hw + 1;
                     x <= Math.min(line + hw - 1, cx); ++x)
                    putHull(x, y, accentC);
            }
        };
        band(i);
        if (rng() < 0.3 && i + 2 < len - 1)
            band(i + 2);
    }

    // The hull shading: a light top edge, a dark bottom edge, the base
    // tone elsewhere (the feature pixels keep their colors).
    for (let i = 0; i < len; ++i) {
        const y = y0 + i;
        for (let x = 0; x <= cx; ++x) {
            const idx = y * W + x;
            if (!hullM[idx] || featM[idx])
                continue;
            const above = (y > 0 && hullM[(y - 1) * W + x]);
            const below = (y + 1 < H && hullM[(y + 1) * W + x]);
            const c = (!above) ? light : (!below ? dark : hullC);
            const j = idx * 4;
            buf[j] = c[0];
            buf[j + 1] = c[1];
            buf[j + 2] = c[2];
            buf[j + 3] = 255;
        }
    }

    // The greeble pass: small marks scattered on the hull surfaces. The
    // detail level sets the count; each mark kind is small so it keeps
    // the 1-pixel pixel-art scale.
    {
        const free = (x, y) => (x >= 0 && x <= cx && y >= 0 && y < H &&
                                hullM[y * W + x] && !featM[y * W + x]);
        const nMark = Math.round(len * detail / 100 * 1.5);
        let nBlock = 0;  // the cargo blocks (a few at most)
        for (let k = 0; k < nMark; ++k) {
            const i = 1 + Math.floor(rng() * (len - 2));
            const cands = [];
            for (const h of hulls)
                if (h.w[i] >= 3)
                    cands.push(h);
            if (!cands.length)
                continue;
            const hull = cands[Math.floor(rng() * cands.length)];
            const line = cx - hull.off;
            const hw = hull.w[i];
            const xlo = Math.max(0, line - hw + 1);
            const xhi = Math.min(cx - 1, line + hw - 1);
            if (xhi <= xlo)
                continue;
            const x0 = xlo + Math.floor(rng() * (xhi - xlo + 1));
            const y = y0 + i;
            const c = (rng() < 0.2) ? accentC : dark;
            const kind = pick(st.greeble);
            if (kind === 0) {
                if (free(x0, y))
                    put(x0, y, c, true);
            } else if (kind === 1) {
                if (free(x0, y) && free(x0, y + 1)) {
                    put(x0, y, c, true);
                    put(x0, y + 1, c, true);
                }
            } else if (kind === 2) {
                const w = 2 + Math.floor(rng() * 3);
                let ok = true;
                for (let x = x0 - w + 1; x <= x0; ++x)
                    if (!free(x, y))
                        ok = false;
                if (ok)
                    for (let x = x0 - w + 1; x <= x0; ++x)
                        put(x, y, c, true);
            } else if (kind === 3) {
                if (free(x0, y) && free(x0 + 1, y) &&
                    free(x0, y + 1) && free(x0 + 1, y + 1)) {
                    put(x0, y, c, true);
                    put(x0 + 1, y, c, true);
                    put(x0, y + 1, c, true);
                    put(x0 + 1, y + 1, c, true);
                }
            } else if (kind === 4) {
                const xe = Math.max(0, line - hw);
                if (free(xe, y))
                    put(xe, y, c, true);
            } else if (kind === 5) {
                const n = 2 + Math.floor(rng() * 3);
                let ok = true;
                for (let j = 0; j < n; ++j)
                    if (!free(x0 - j, y - j))
                        ok = false;
                if (ok)
                    for (let j = 0; j < n; ++j)
                        put(x0 - j, y - j, c, true);
            } else if (kind === 6) {  // a porthole: a small round window
                const r = (unit >= 2) ? 2 : 1;
                let ok = true;
                for (let dy = -r; dy <= r && ok; ++dy)
                    for (let dx = -r; dx <= r; ++dx)
                        if (dx * dx + dy * dy <= r * r &&
                            !free(x0 + dx, y + dy)) {
                            ok = false;
                            break;
                        }
                if (ok) {
                    for (let dy = -r; dy <= r; ++dy)
                        for (let dx = -r; dx <= r; ++dx)
                            if (dx * dx + dy * dy <= r * r)
                                put(x0 + dx, y + dy, c, true);
                    if (r >= 2)
                        put(x0, y, accentC, true);
                }
            } else if (nBlock < 2) {  // a cargo block: a recessed hatch at the hull edge
                const xe = Math.max(0, line - hw);
                const wb = Math.max(2, unit);
                const hb = Math.max(2, unit);
                let ok = true;
                for (let dy = 0; dy < hb && ok; ++dy)
                    for (let dx = 0; dx < wb; ++dx)
                        if (!free(xe + dx, y + dy)) {
                            ok = false;
                            break;
                        }
                if (ok) {
                    for (let dy = 0; dy < hb; ++dy)
                        for (let dx = 0; dx < wb; ++dx)
                            put(xe + dx, y + dy, dark, true);
                    put(xe, y, accentC, true);
                    ++nBlock;
                }
            }
        }
    }

    // Mirror the left half onto the right.
    for (let y = 0; y < H; ++y) {
        for (let x = 0; x <= cx; ++x) {
            const s = (y * W + x) * 4;
            const d = (y * W + (W - 1 - x)) * 4;
            buf[d] = buf[s];
            buf[d + 1] = buf[s + 1];
            buf[d + 2] = buf[s + 2];
            buf[d + 3] = buf[s + 3];
        }
    }
    // Keep a 1px clear margin at the canvas edge (a feature that would
    // reach it is clipped rather than cut by the document border).
    for (let y = 0; y < H; ++y) {
        buf[y * W * 4 + 3] = 0;
        buf[y * W * 4 + (W - 1) * 4 + 3] = 0;
    }
    return buf;
}

// Build the backdrop into an RGBA buffer (row-major, opaque). When the
// style is "auto" (or "random") the seed picks one; the Perlin field and
// the feature placement use their own seeded streams, so the same seed
// always gives the same backdrop (and never touches the ship's draws).
function buildBackground(styleId, seed, W, H, detail) {
    const rng = mulberry32((seed ^ 0x5EEDB0B9) >>> 0);
    const pick = (arr) => arr[Math.floor(rng() * arr.length)];
    const names = ["space", "terrain", "water"];
    const sid = (styleId === "auto" || styleId === "random")
        ? pick(names)
        : (BGS[styleId] ? styleId : "space");
    const st = BGS[sid];
    const noise = perlin2((seed ^ 0xA11CE5) >>> 0);
    const bands = st.bands;

    const buf = new Uint8Array(W * H * 4);

    // Write a pixel (the backdrop is a plain image: no mirror, no hull
    // mask).
    function put(x, y, c) {
        if (x < 0 || x >= W || y < 0 || y >= H)
            return;
        const i = (y * W + x) * 4;
        buf[i] = c[0];
        buf[i + 1] = c[1];
        buf[i + 2] = c[2];
        buf[i + 3] = 255;
    }

    // The band edges in t space (0..1): each band gets a random width
    // (bandW is a min/max weight), skewed by a per-seed bias (bias^i),
    // so some backdrops favor the light bands and some the dark ones.
    const bias = st.bias[0] + rng() * (st.bias[1] - st.bias[0]);
    const widths = [];
    let total = 0;
    for (let i = 0; i < bands.length; ++i) {
        const w = (st.bandW[0] + rng() * (st.bandW[1] - st.bandW[0])) *
                  Math.pow(bias, i);
        widths.push(w);
        total += w;
    }
    const e = [0];
    for (let i = 0; i < bands.length; ++i)
        e.push(e[i] + widths[i] / total);

    // The color for a noise value t (0..1): the band it falls in, with a
    // transition (st.trans, a fraction of the band width) into the next
    // band near the edge (0 = a hard seam).
    function colorAt(t) {
        let i = 0;
        while (i < bands.length - 1 && t >= e[i + 1])
            ++i;
        const u = (t - e[i]) / (e[i + 1] - e[i]);
        if (st.trans > 0 && i < bands.length - 1 && u > 1 - st.trans)
            return mix(bands[i], bands[i + 1],
                       (u - (1 - st.trans)) / st.trans);
        return bands[i];
    }

    // The base texture: the fbm field mapped through the band colors
    // (water stretches the field for horizontal ripples).
    for (let y = 0; y < H; ++y)
        for (let x = 0; x < W; ++x) {
            const n = fbm(noise, x * st.scale, y * st.scale * st.stretch,
                           st.oct);
            const t = Math.max(0, Math.min(1, (n + 1) / 2));
            const c = colorAt(t);
            const i = (y * W + x) * 4;
            buf[i] = c[0];
            buf[i + 1] = c[1];
            buf[i + 2] = c[2];
            buf[i + 3] = 255;
        }

    // The nebulae (a space backdrop): 2-3 soft gas clouds of the accent
    // hues, masked by the noise so the edges read as gas, not discs.
    if (sid === "space") {
        const nNeb = 2 + Math.floor(rng() * 2);
        for (let k = 0; k < nNeb; ++k) {
            const nc = st.accent[k % st.accent.length];
            const r = Math.round(Math.min(W, H) * (0.12 + 0.10 * rng()));
            const x0 = Math.round(W * (0.15 + 0.70 * rng()));
            const y0 = Math.round(H * (0.15 + 0.70 * rng()));
            for (let dy = -r; dy <= r; ++dy) {
                for (let dx = -r; dx <= r; ++dx) {
                    const d = Math.sqrt(dx * dx + dy * dy) / r;
                    if (d > 1)
                        continue;
                    const x = x0 + dx, y = y0 + dy;
                    if (x < 0 || x >= W || y < 0 || y >= H)
                        continue;
                    const n = fbm(noise, x * st.scale * 1.7,
                                  y * st.scale * 1.7, 2);
                    const t = Math.max(0, (n + 1) / 2 - 0.35) / 0.65;
                    const a = (1 - d) * (1 - d) * t * 0.6;
                    if (a < 0.02)
                        continue;
                    const i = (y * W + x) * 4;
                    buf[i] = Math.round(buf[i] + (nc[0] - buf[i]) * a);
                    buf[i + 1] = Math.round(
                        buf[i + 1] + (nc[1] - buf[i + 1]) * a);
                    buf[i + 2] = Math.round(
                        buf[i + 2] + (nc[2] - buf[i + 2]) * a);
                }
            }
        }
    }

    // The features: small marks scattered over the backdrop (the detail
    // level sets the count, like the ship's greeble pass).
    {
        const starC = [[255, 255, 255], [212, 226, 255], [255, 240, 200]];
        const nFeat = Math.round(st.featN * detail / 50);
        for (let k = 0; k < nFeat; ++k) {
            const x = 1 + Math.floor(rng() * (W - 2));
            const y = 1 + Math.floor(rng() * (H - 2));
            const kind = pick(st.feats);
            if (kind === "star") {
                put(x, y, pick(starC));
            } else if (kind === "sparkle") {
                put(x, y, [255, 255, 255]);
                put(x - 1, y, [168, 186, 232]);
                put(x + 1, y, [168, 186, 232]);
                put(x, y - 1, [168, 186, 232]);
                put(x, y + 1, [168, 186, 232]);
            } else if (kind === "rubble") {
                const w = 2 + Math.floor(rng() * 2);
                const h = 1 + Math.floor(rng() * 2);
                for (let dy = 0; dy < h; ++dy)
                    for (let dx = 0; dx < w; ++dx)
                        if (rng() < 0.8)
                            put(x - (w >> 1) + dx, y - (h >> 1) + dy,
                                [58, 44, 26]);
            } else if (kind === "tuft") {
                put(x, y, st.accent[1]);
                put(x, y - 1, st.accent[1]);
                if (rng() < 0.5)
                    put(x + 1, y - 1, st.accent[1]);
            } else if (kind === "pebble") {
                put(x, y, st.accent[0]);
            } else if (kind === "foam") {
                const w = 1 + Math.floor(rng() * 3);
                for (let dx = 0; dx < w; ++dx)
                    put(x - (w >> 1) + dx, y, st.accent[0]);
            } else {  // patch: a darker water patch
                put(x, y, st.accent[1]);
                put(x + 1, y, st.accent[1]);
                put(x, y + 1, st.accent[1]);
                if (rng() < 0.5)
                    put(x + 1, y + 1, st.accent[1]);
            }
        }
    }
    return buf;
}

// Place the ship buffer (the H x H buildShip output) on a transparent
// canvas with a uniform margin around it.
function composeShip(ship, size, pad) {
    const {W, H} = shipSize(size);
    const P = Math.max(0, Math.round(pad));
    const W2 = W + 2 * P;
    const out = new Uint8Array(W2 * W2 * 4);
    for (let y = 0; y < H; ++y) {
        const s4 = (y * W) * 4;
        const d4 = ((y + P) * W2) * 4;
        for (let x = 0; x < W; ++x) {
            const s = s4 + x * 4;
            const d = d4 + (x + P) * 4;
            out[d] = ship[s];
            out[d + 1] = ship[s + 1];
            out[d + 2] = ship[s + 2];
            out[d + 3] = ship[s + 3];
        }
    }
    return out;
}

// Convert a storage value (the GUI RGBA int or the CLI hex string) to
// [r, g, b]; the fallback is used when the value is missing or bad.
function colorParam(value, fallback) {
    if (typeof value === "number" && value !== 0)
        return [value & 0xFF, (value >> 8) & 0xFF, (value >> 16) & 0xFF];
    const hex = String(value === null || value === undefined
                       ? "" : value).trim().replace(/^#/, "");
    if (/^[0-9a-fA-F]{6}$/.test(hex))
        return [parseInt(hex.slice(0, 2), 16),
                parseInt(hex.slice(2, 4), 16),
                parseInt(hex.slice(4, 6), 16)];
    return colorParam(fallback, fallback);
}

const KEYS = ["style", "seed", "size", "detail", "hull", "accent", "glow",
              "bg", "pad"];
const DEFAULTS = {style: "random", seed: 0, size: 48, detail: 50,
                  hull: "8c96a5", accent: "ffa300", glow: true,
                  bg: "none", pad: 0};
const BG_NAMES = ["none", "auto", "random", "space", "terrain", "water"];

// Read the parameters from storage: a --set value (CLI) or a saved value
// (the GUI persists them when you click Generate). --set values are
// strings, so the numbers are coerced.
function readParams() {
    for (const key of KEYS)
        storage.load(key);

    const seed = parseInt(storage.get("seed"), 10);
    const size = parseInt(storage.get("size"), 10);
    const detail = parseInt(storage.get("detail"), 10);
    const pad = parseInt(storage.get("pad"), 10);
    const glow = storage.get("glow");
    const bg = storage.get("bg");
    return {
        style: storage.get("style") || DEFAULTS.style,
        seed: isNaN(seed) ? DEFAULTS.seed : seed,
        size: isNaN(size) ? DEFAULTS.size : size,
        detail: isNaN(detail) ? DEFAULTS.detail : detail,
        hull: colorParam(storage.get("hull"), DEFAULTS.hull),
        accent: colorParam(storage.get("accent"), DEFAULTS.accent),
        glow: (glow === null || glow === undefined) ? DEFAULTS.glow
             : (glow === true || glow === "true"),
        bg: (BG_NAMES.indexOf(bg) >= 0) ? bg : DEFAULTS.bg,
        pad: isNaN(pad) ? DEFAULTS.pad
             : Math.max(0, Math.min(32, pad)),
    };
}

// Create a new document with the ship in it. When out is given (the
// command line path) it is saved there as a copy. The backdrop (when
// picked) goes on its own layer below the ship.
function generate(styleId, seed, size, detail, hullC, accentC, glow, bg,
                  pad, out) {
    const {W, H} = shipSize(size);
    const P = Math.max(0, Math.round(pad));
    const W2 = W + 2 * P;
    if (command.NewFile({width: W2, height: W2,
                         "color-mode": "rgb",
                         "background": "transparent"}) === 0) {
        console.log("spaceship_gen: could not create the sprite");
        return;
    }
    const buf = composeShip(
        buildShip(styleId, seed, size, detail, hullC, accentC, glow),
        size, P);
    try {
        // One undo step for the whole document (a failure rolls the
        // group back).
        app.transaction(() => {
            if (bg === "none") {
                app.activeImage.putImageData(buf);
            } else {
                sprite.layer(0).name = "background";
                sprite.layer(0).cel(0).image.putImageData(
                    buildBackground(bg, seed, W2, W2, detail));
                if (command.NewLayer({name: "ship"}) === 0)
                    throw new Error("could not create the ship layer");
                sprite.layer(1).createCel(0);
                sprite.layer(1).cel(0).image.putImageData(buf);
            }
        });
    } catch (e) {
        console.log("spaceship_gen: " + e);
        return;
    }
    if (out) {
        sprite.saveAs(out, true);
        console.log("spaceship_gen: wrote " + out);
    }
}

// Render the ship into the dialog image view (nearest-neighbor scaled to
// fit the 160 px preview; the backdrop fills the canvas when one is
// picked, otherwise a dark backdrop stands in for the transparent
// document background).
function preview(styleId, seed, size, detail, hullC, accentC, glow, bg,
                 pad) {
    if (!view)
        return;
    const {W, H} = shipSize(size);
    const P = Math.max(0, Math.round(pad));
    const W2 = W + 2 * P;
    const ship = composeShip(
        buildShip(styleId, seed, size, detail, hullC, accentC, glow),
        size, P);
    const back = (bg === "none")
        ? null
        : buildBackground(bg, seed, W2, W2, detail);
    // Integer blocks when the canvas fits the preview, nearest-neighbor
    // sampling when it is bigger (the view draws 1:1 and never shrinks).
    const s = Math.floor(160 / W2);
    const outW = (s > 0) ? W2 * s : 160;
    const out = new Uint8Array(outW * outW * 4);
    for (let i = 0; i < out.length; i += 4) {
        out[i] = 24;
        out[i + 1] = 24;
        out[i + 2] = 28;
        out[i + 3] = 255;
    }
    for (let y = 0; y < outW; ++y) {
        const sy = (s > 0) ? Math.floor(y / s) : Math.floor(y * W2 / outW);
        for (let x = 0; x < outW; ++x) {
            const sx = (s > 0) ? Math.floor(x / s) : Math.floor(x * W2 / outW);
            const s4 = (sy * W2 + sx) * 4;
            const d4 = (y * outW + x) * 4;
            if (back) {
                out[d4] = back[s4];
                out[d4 + 1] = back[s4 + 1];
                out[d4 + 2] = back[s4 + 2];
                out[d4 + 3] = 255;
            }
            if (ship[s4 + 3] === 255) {
                out[d4] = ship[s4];
                out[d4 + 1] = ship[s4 + 1];
                out[d4 + 2] = ship[s4 + 2];
                out[d4 + 3] = 255;
            }
        }
    }
    view.putImageData(out, outW, outW);
}

let dialog, styleW, seedW, sizeW, detailW, hullW, accentW, glowW, bgW,
    padW, view, loreW;

if (!app.isHeadless) {
    // GUI mode: a dialog with the parameters; the preview updates as the
    // inputs change and Generate opens the ship in a new document.
    const pc = app.pixelColor;
    dialog = app.createDialog("spaceship_gen");
    dialog.title = "Spaceship Generator";

    dialog.addLabel("Style:", "style-label");
    dialog.addDropDown("style");
    dialog.addBreak();

    dialog.addIntEntry("Seed:", "seed", 0, 0xFFFFF);
    dialog.addButton("Random", "random");
    dialog.addBreak();

    dialog.addIntEntry("Size:", "size", 16, 128);
    dialog.addBreak();

    dialog.addLabel("Detail:");
    dialog.addSlider("detail", 0, 100);
    dialog.addBreak();

    dialog.addLabel("Hull:");
    dialog.addColor("hull", pc.rgba(140, 150, 165, 255));
    dialog.addBreak();

    dialog.addLabel("Accent:");
    dialog.addColor("accent", pc.rgba(255, 163, 0, 255));
    dialog.addBreak();

    dialog.addCheckBox("Engine glow:", "glow");
    dialog.addBreak();

    dialog.addLabel("Background:");
    dialog.addDropDown("bg");
    dialog.addBreak();

    dialog.addIntEntry("Padding:", "pad", 0, 32);
    dialog.addBreak();

    dialog.addButton("Generate", "generate");
    dialog.addBreak();

    dialog.addImageView("preview-img");

    // The ship's lore (generated from the seed and the style); the text
    // is set in previewNow(), which wraps it (the text box does not wrap
    // on its own).
    dialog.addTextBox("", "lore");

    styleW = dialog.get("style");
    seedW = dialog.get("seed");
    sizeW = dialog.get("size");
    detailW = dialog.get("detail");
    hullW = dialog.get("hull");
    accentW = dialog.get("accent");
    glowW = dialog.get("glow");
    bgW = dialog.get("bg");
    padW = dialog.get("pad");
    view = dialog.get("preview-img");
    loreW = dialog.get("lore");

    for (const name of ["random", "fighter", "cruiser", "shuttle"])
        styleW.addOption(name);
    for (const name of ["none", "auto", "space", "terrain", "water"])
        bgW.addOption(name);

    // Restore the persisted settings (if any) into the widgets.
    const p = readParams();
    styleW.value = p.style;
    seedW.value = p.seed;
    sizeW.value = p.size;
    detailW.value = p.detail;
    glowW.checked = p.glow;
    bgW.value = p.bg;
    padW.value = p.pad;
    hullW.color = pc.rgba(p.hull[0], p.hull[1], p.hull[2], 255);
    accentW.color = pc.rgba(p.accent[0], p.accent[1], p.accent[2], 255);
    previewNow();
} else {
    // Command line mode: the parameters come from the --set values.
    const p = readParams();
    const out = storage.get("out");
    if (!out) {
        console.log("spaceship_gen: missing output file " +
                    "(use --set out=<file>)");
    } else {
        generate(p.style, p.seed, p.size, p.detail, p.hull, p.accent,
                 p.glow, p.bg, p.pad, String(out));
    }
}

// Roll a new random seed into the entry.
function rollSeed() {
    seedW.value = 1 + Math.floor(Math.random() * 0xFFFFF);
}

// Seed 0 picks a random seed and shows it in the entry (the ship can
// then be regenerated from the entry value).
function nextSeed() {
    if (seedW.value === 0)
        rollSeed();
    return seedW.value;
}

function previewNow() {
    preview(styleW.value, seedW.value, sizeW.value, detailW.value,
            colorParam(hullW.color, DEFAULTS.hull),
            colorParam(accentW.color, DEFAULTS.accent),
            glowW.checked, bgW.value, padW.value);
    // The lore follows the preview (the same seed and style tell the
    // same tale).
    loreW.text = wrapText(genLore(styleW.value, seedW.value), 50);
}

function onEvent(e) {
    if (!dialog)
        return;
    if (e === "generate_click") {
        generate(styleW.value, nextSeed(), sizeW.value, detailW.value,
                 colorParam(hullW.color, DEFAULTS.hull),
                 colorParam(accentW.color, DEFAULTS.accent),
                 glowW.checked, bgW.value, padW.value, "");
        // The seed entry write above is silent, so refresh the preview
        // with the rolled seed.
        previewNow();
        // Persist the settings so a command line run can reuse them.
        for (const key of KEYS)
            storage.save(key);
    } else if (e === "random_click") {
        rollSeed();
        previewNow();
    } else if (e === "style_change" || e === "seed_change" ||
               e === "size_change" || e === "detail_change" ||
               e === "hull_change" || e === "accent_change" ||
               e === "glow_click" || e === "bg_change" ||
               e === "pad_change") {
        previewNow();
    } else if (e === "spaceship_gen_close") {
        dialog.close();
    }
}
