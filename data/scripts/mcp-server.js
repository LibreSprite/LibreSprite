// LibreSprite
// Copyright (C) 2026  LibreSprite contributors
//
// MCP (Model Context Protocol) server, Streamable HTTP transport,
// 2025-03-26 and 2025-06-18 revisions. Runs inside LibreSprite on the
// built-in HTTPServer script API.
//
// Run it from the Scripts menu or with `libresprite --script`. The
// server lives while the app window is open; the console prints the
// endpoint URL. Point any MCP client at that URL.
//
// Limitations:
//  - GUI mode only: the engine tick pump runs with the UI loop.
//  - JSON responses only: no SSE streams, no sessions, no auth.
//  - Binds to localhost.
//  - run_command may open dialogs that need user interaction.
//  - Script edits (pixel_set, palette_set) write pixels directly: they
//    create no undo history and the canvas may not repaint immediately.
//    Back up with sprite_save_as before large edits.

// --- config ---

const PORT = 0; // 0 = first free port in 9000-9099
const READ_ONLY = false; // if true, the mutating tools are disabled

// --- protocol constants ---

const PROTOCOL_VERSIONS = ["2025-03-26", "2025-06-18"];
const LATEST_VERSION = "2025-06-18";
const SERVER_NAME = "libresprite-mcp";

// --- JSON-RPC 2.0 ---

function rpcResult(id, result) {
  return { jsonrpc: "2.0", id: id, result: result };
}

function rpcError(id, code, message) {
  return { jsonrpc: "2.0", id: id, error: { code: code, message: message } };
}

// A protocol-level failure (bad params) that must become a JSON-RPC error
// instead of a tool error result.
class RpcError extends Error {
  constructor(code, message) {
    super(message);
    this.code = code;
  }
}

// --- color helpers ---

const col = app.pixelColor;

function colorToHex(c) {
  const h = (n) => ("0" + (n & 0xff).toString(16)).slice(-2);
  return "#" + h(col.rgbaR(c)) + h(col.rgbaG(c)) + h(col.rgbaB(c)) + h(col.rgbaA(c));
}

// Accepts "#RRGGBB", "#RRGGBBAA" or {r,g,b,a} with 0-255 values.
function parseColor(value) {
  if (typeof value === "string") {
    const m = /^#?([0-9a-f]{6})([0-9a-f]{2})?$/i.exec(value.trim());
    if (!m)
      throw new Error("invalid color " + value + " (use #RRGGBB or #RRGGBBAA)");
    const r = parseInt(m[1].slice(0, 2), 16);
    const g = parseInt(m[1].slice(2, 4), 16);
    const b = parseInt(m[1].slice(4, 6), 16);
    const a = m[2] ? parseInt(m[2], 16) : 255;
    return col.rgba(r, g, b, a);
  }
  if (value && typeof value === "object") {
    const parts = [["r", value.r], ["g", value.g], ["b", value.b],
                   ["a", value.a !== undefined ? value.a : 255]];
    for (const [name, v] of parts) {
      if (typeof v !== "number" || v < 0 || v > 255)
        throw new Error("invalid color component " + name);
    }
    return col.rgba(value.r | 0, value.g | 0, value.b | 0,
                    (value.a !== undefined ? value.a : 255) | 0);
  }
  throw new Error("color must be a hex string or an {r,g,b,a} object");
}

// --- tool helpers ---

function textResult(text) {
  return [{ type: "text", text: text }];
}

function imageResult(data, mimeType) {
  return [{ type: "image", data: data, mimeType: mimeType }];
}

function getSprite() {
  const sprite = app.activeSprite;
  if (!sprite)
    throw new Error("no document is open in LibreSprite");
  return sprite;
}

// Resolves a cel from optional 0-based frame/layer args.
function getCel(args) {
  const sprite = getSprite();
  const layerIndex = args.layer !== undefined ? args.layer : app.activeLayerNumber;
  const frameIndex = args.frame !== undefined ? args.frame : app.activeFrameNumber;
  if (!Number.isInteger(layerIndex) || layerIndex < 0 || layerIndex >= sprite.layerCount)
    throw new Error("layer " + layerIndex + " out of range (0.." + (sprite.layerCount - 1) + ")");
  const layer = sprite.layer(layerIndex);
  if (!Number.isInteger(frameIndex) || frameIndex < 0 || frameIndex >= layer.celCount)
    throw new Error("frame " + frameIndex + " out of range (0.." + (layer.celCount - 1) + ")");
  const cel = layer.cel(frameIndex);
  if (!cel || !cel.image)
    throw new Error("layer " + layerIndex + " has no image at frame " + frameIndex);
  return cel;
}

function checkXY(image, args) {
  const x = args.x, y = args.y;
  if (!Number.isInteger(x) || !Number.isInteger(y) ||
      x < 0 || y < 0 || x >= image.width || y >= image.height)
    throw new Error("pixel (" + x + "," + y + ") out of bounds (image is " +
                    image.width + "x" + image.height + ")");
  return { x: x, y: y };
}

function checkWritable() {
  if (READ_ONLY)
    throw new Error("server is running in read-only mode");
}

// --- tools ---

const frameOrLayerSchema = {
  frame: { type: "integer", description: "0-based frame index (default: active frame)" },
  layer: { type: "integer", description: "0-based layer index (default: active layer)" },
};

const TOOLS = [
  {
    name: "sprite_info",
    description: "Info about the active sprite: file name, size, color mode, layer and frame counts, active layer/frame, platform and app version. Call this first to see what is open.",
    inputSchema: { type: "object", properties: {} },
    run: () => {
      const sprite = getSprite();
      // The sprite frame count is the largest cel count across layers.
      let frameCount = 0;
      for (let i = 0; i < sprite.layerCount; ++i)
        frameCount = Math.max(frameCount, sprite.layer(i).celCount);
      const modeNames = {
        [ColorMode.RGB]: "rgb",
        [ColorMode.GRAYSCALE]: "grayscale",
        [ColorMode.INDEXED]: "indexed",
        [ColorMode.BITMAP]: "bitmap",
      };
      const info = {
        filename: sprite.filename,
        width: sprite.width,
        height: sprite.height,
        colorMode: modeNames[sprite.colorMode] !== undefined ? modeNames[sprite.colorMode] : sprite.colorMode,
        layerCount: sprite.layerCount,
        frameCount: frameCount,
        activeLayer: app.activeLayerNumber,
        activeFrame: app.activeFrameNumber,
        platform: app.platform,
        appVersion: app.version,
      };
      return textResult(JSON.stringify(info, null, 2));
    },
  },
  {
    name: "sprite_image",
    description: "Returns the given frame as a PNG image so you can see the current art. Omit frame/layer to use the active ones. Use this to check a drawing before saving.",
    inputSchema: { type: "object", properties: frameOrLayerSchema },
    run: (args) => {
      const cel = getCel(args);
      let png = cel.image.getPNGData();
      // The binding returns a data-URL; MCP image content wants raw base64.
      const prefix = "data:image/png;base64,";
      if (png && png.indexOf(prefix) === 0)
        png = png.slice(prefix.length);
      if (!png)
        throw new Error("PNG encoding failed (empty data)");
      return imageResult(png, "image/png");
    },
  },
  {
    name: "pixel_get",
    description: "Reads the color of one pixel as #RRGGBBAA. On indexed-color sprites the palette color is resolved automatically.",
    inputSchema: {
      type: "object",
      properties: {
        x: { type: "integer", description: "0-based column" },
        y: { type: "integer", description: "0-based row" },
        ...frameOrLayerSchema,
      },
      required: ["x", "y"],
    },
    run: (args) => {
      const sprite = getSprite();
      const cel = getCel(args);
      const p = checkXY(cel.image, args);
      let color = cel.image.getPixel(p.x, p.y);
      if (sprite.colorMode === ColorMode.INDEXED || sprite.colorMode === ColorMode.BITMAP) {
        // Indexed and bitmap images return the palette index, not a color.
        const pal = sprite.palette;
        if (color >= pal.length)
          throw new Error("pixel " + p.x + "," + p.y + " holds index " + color +
            " which is outside the palette");
        color = pal.get(color);
      } else if (sprite.colorMode === ColorMode.GRAYSCALE) {
        const v = col.grayaV(color), a = col.grayaA(color);
        color = col.rgba(v, v, v, a);
      }
      return textResult(colorToHex(color));
    },
  },
  {
    name: "pixel_set",
    description: "Sets one pixel on the given frame. color is #RRGGBB, #RRGGBBAA or {r,g,b,a} with 0-255 values. On indexed-color sprites the color must exist in the palette. Edits are not undoable; use sprite_save_as to back up before large edits. Check the result with sprite_image.",
    inputSchema: {
      type: "object",
      properties: {
        x: { type: "integer", description: "0-based column" },
        y: { type: "integer", description: "0-based row" },
        color: { description: "#RRGGBB, #RRGGBBAA or {r,g,b,a} with 0-255 values" },
        ...frameOrLayerSchema,
      },
      required: ["x", "y", "color"],
    },
    run: (args) => {
      checkWritable();
      const sprite = getSprite();
      const cel = getCel(args);
      const p = checkXY(cel.image, args);
      const color = parseColor(args.color);
      if (sprite.colorMode === ColorMode.INDEXED || sprite.colorMode === ColorMode.BITMAP) {
        // Indexed and bitmap images store palette indices, not colors.
        const pal = sprite.palette;
        let index = -1;
        for (let i = 0; i < pal.length; ++i)
          if (pal.get(i) === color) { index = i; break; }
        if (index < 0)
          throw new Error(colorToHex(color) + " is not in the palette; pick a palette color (palette_get) or add it (palette_set)");
        cel.image.putPixel(p.x, p.y, index);
        return textResult("set pixel " + p.x + "," + p.y + " to " + colorToHex(color) +
          " (palette index " + index + ")");
      }
      cel.image.putPixel(p.x, p.y, color);
      return textResult("set pixel " + p.x + "," + p.y + " to " + colorToHex(color));
    },
  },
  {
    name: "palette_get",
    description: "Returns the sprite palette as a list of #RRGGBBAA colors, index 0 first.",
    inputSchema: { type: "object", properties: {} },
    run: () => {
      const pal = getSprite().palette;
      const colors = [];
      for (let i = 0; i < pal.length; ++i)
        colors.push(colorToHex(pal.get(i)));
      return textResult(JSON.stringify(colors));
    },
  },
  {
    name: "palette_set",
    description: "Sets one palette entry by index (0-based). color is #RRGGBB, #RRGGBBAA or {r,g,b,a} with 0-255 values.",
    inputSchema: {
      type: "object",
      properties: {
        index: { type: "integer", description: "0-based palette entry" },
        color: { description: "#RRGGBB, #RRGGBBAA or {r,g,b,a} with 0-255 values" },
      },
      required: ["index", "color"],
    },
    run: (args) => {
      checkWritable();
      const pal = getSprite().palette;
      const index = args.index;
      if (!Number.isInteger(index) || index < 0 || index >= pal.length)
        throw new Error("palette index " + index + " out of range (0.." + (pal.length - 1) + ")");
      const color = parseColor(args.color);
      pal.set(index, col.rgbaR(color), col.rgbaG(color), col.rgbaB(color), col.rgbaA(color));
      return textResult("set palette entry " + index + " to " + colorToHex(color));
    },
  },
  {
    name: "layers_list",
    description: "Lists the sprite layers (index 0 is the bottom layer): name, visibility, lock state, kind and cel count.",
    inputSchema: { type: "object", properties: {} },
    run: () => {
      const sprite = getSprite();
      const layers = [];
      for (let i = 0; i < sprite.layerCount; ++i) {
        const layer = sprite.layer(i);
        layers.push({
          index: i,
          name: layer.name,
          visible: layer.isVisible,
          editable: layer.isEditable,
          kind: layer.isImage ? (layer.isBackground ? "background" : "image") : "folder",
          celCount: layer.celCount,
        });
      }
      return textResult(JSON.stringify(layers, null, 2));
    },
  },
  {
    name: "sprite_save",
    description: "Saves the active document to its current file. Use sprite_save_as to write to a new path.",
    inputSchema: { type: "object", properties: {} },
    run: () => {
      checkWritable();
      const sprite = getSprite();
      if (!sprite.filename)
        throw new Error("the document has no file name; use sprite_save_as");
      sprite.save();
      return textResult("saved " + sprite.filename);
    },
  },
  {
    name: "sprite_save_as",
    description: "Saves a copy of the active document to the given path; the document keeps its current name. Use a supported extension (.ase for full data, .png for a flattened image).",
    inputSchema: {
      type: "object",
      properties: {
        path: { type: "string", description: "destination file path" },
      },
      required: ["path"],
    },
    run: (args) => {
      checkWritable();
      const sprite = getSprite();
      const path = args.path;
      if (typeof path !== "string" || !path)
        throw new Error("path must be a non-empty string");
      sprite.saveAs(path, true);
      return textResult("saved a copy to " + path);
    },
  },
  {
    name: "run_command",
    description: "Runs a LibreSprite UI command by id (for example NewLayer, Undo, GotoNextFrame). Some commands open dialogs that wait for the user; prefer the dedicated tools. CloseFile, CloseAllFiles and Exit are not exposed.",
    inputSchema: {
      type: "object",
      properties: {
        name: { type: "string", description: "command id" },
        params: { type: "object", description: "optional command parameters, values as strings" },
      },
      required: ["name"],
    },
    run: (args) => {
      checkWritable();
      const name = args.name;
      if (typeof name !== "string" || !name)
        throw new Error("name must be a non-empty string");
      const fn = app.command[name];
      if (typeof fn !== "function")
        throw new Error("unknown command " + name + ". Available: " + listCommandIds().join(", "));
      const params = args.params;
      // Command methods are class methods: keep the command object as this.
      const ok = (params && typeof params === "object") ? fn.call(app.command, params) : fn.call(app.command);
      if (!ok)
        throw new Error("command " + name + " is not available in the current state");
      return textResult("ran " + name);
    },
  },
];

const TOOLS_BY_NAME = {};
for (const t of TOOLS)
  TOOLS_BY_NAME[t.name] = t;

// Command ids live on the command object prototype.
function listCommandIds() {
  try {
    const proto = Object.getPrototypeOf(app.command);
    return Object.getOwnPropertyNames(proto)
      .filter((k) => k !== "constructor" && k !== "setParameter" && k !== "clearParameters");
  } catch (e) {
    return [];
  }
}

// HTTP header names are case-insensitive and the server keeps the sent case.
function getHeader(req, name) {
  const lower = name.toLowerCase();
  for (const key in req.headers)
    if (key.toLowerCase() === lower)
      return req.headers[key];
  return undefined;
}

// --- MCP method handlers ---

function handleInitialize(params) {
  const requested = params && params.protocolVersion;
  return {
    protocolVersion: (requested && PROTOCOL_VERSIONS.indexOf(requested) >= 0) ? requested : LATEST_VERSION,
    capabilities: { tools: { listChanged: false } },
    serverInfo: { name: SERVER_NAME, version: LS.version },
    instructions: "Tools act on the document active in LibreSprite. Open a sprite first, then use sprite_info to inspect it. Use sprite_image to see frames as PNG. Edits are not undoable, so back up with sprite_save_as before large edits. run_command may open dialogs that need the user.",
  };
}

function handleToolCall(params) {
  if (!params || typeof params.name !== "string")
    throw new RpcError(-32602, "tools/call params.name (the tool name) is required");
  const tool = TOOLS_BY_NAME[params.name];
  if (!tool)
    return {
      content: textResult("unknown tool " + params.name +
        ". Available: " + TOOLS.map((t) => t.name).join(", ")),
      isError: true,
    };
  const args = (params.arguments && typeof params.arguments === "object") ? params.arguments : {};
  try {
    return { content: tool.run(args) };
  } catch (e) {
    if (e instanceof RpcError)
      throw e;
    return { content: textResult(e && e.message ? e.message : String(e)), isError: true };
  }
}

function dispatch(msg) {
  const id = msg.id;
  try {
    switch (msg.method) {
      case "initialize":
        return rpcResult(id, handleInitialize(msg.params));
      case "tools/list":
        return rpcResult(id, {
          tools: TOOLS.map((t) => ({
            name: t.name,
            description: t.description,
            inputSchema: t.inputSchema,
          })),
        });
      case "tools/call":
        return rpcResult(id, handleToolCall(msg.params));
      case "ping":
        return rpcResult(id, {});
      case "resources/list":
        return rpcResult(id, { resources: [] });
      case "prompts/list":
        return rpcResult(id, { prompts: [] });
      default:
        return rpcError(id, -32601, "method not found: " + msg.method);
    }
  } catch (e) {
    if (e instanceof RpcError)
      return rpcError(id, e.code, e.message);
    return rpcError(id, -32603, "internal error: " + (e && e.message ? e.message : String(e)));
  }
}

// --- HTTP handlers ---

const CORS_HEADERS = {
  "Access-Control-Allow-Origin": "*",
  "Access-Control-Allow-Methods": "POST, GET, OPTIONS, DELETE",
  "Access-Control-Allow-Headers": "Content-Type, Accept, MCP-Protocol-Version, Mcp-Session-Id",
  "Access-Control-Max-Age": "86400",
};

// DNS rebinding guard: a browser Origin must point at localhost.
const LOCAL_HOSTS = { localhost: 1, "127.0.0.1": 1, "::1": 1 };

function originHost(origin) {
  let m = /^https?:\/\/\[(.+)\]/i.exec(origin);
  if (m)
    return m[1].toLowerCase();
  m = /^https?:\/\/([^/:]+)/i.exec(origin);
  return m ? m[1].toLowerCase() : null;
}

function originBlock(req) {
  const origin = getHeader(req, "origin");
  if (!origin || origin === "null")
    return null;
  const host = originHost(origin);
  if (!host || !LOCAL_HOSTS[host])
    return { status: 403, body: "Forbidden: non-local Origin" };
  return null;
}

function jsonRpcHttp(status, obj) {
  return {
    status: status,
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify(obj),
  };
}

function handlePost(req) {
  const blocked = originBlock(req);
  if (blocked)
    return blocked;
  if (req.path !== "/mcp")
    return jsonRpcHttp(404, rpcError(null, -32600, "not found; use /mcp"));

  let msg;
  try {
    // __utf8decode is the engine's byte-array to string helper
    msg = JSON.parse(__utf8decode(req.body));
  } catch (e) {
    return jsonRpcHttp(400, rpcError(null, -32700, "parse error"));
  }
  if (!msg || typeof msg !== "object" || Array.isArray(msg))
    return jsonRpcHttp(400, rpcError(null, -32600,
      "invalid request: a single JSON-RPC object is required"));
  if (msg.jsonrpc !== "2.0")
    return jsonRpcHttp(400, rpcError(msg.id === undefined ? null : msg.id,
      -32600, "invalid request: jsonrpc must be \"2.0\""));

  // The version header is checked on every request except initialize:
  // initialize negotiates the version in the body instead.
  const version = getHeader(req, "mcp-protocol-version");
  if (msg.method !== "initialize" && version && PROTOCOL_VERSIONS.indexOf(version) < 0)
    return jsonRpcHttp(400, rpcError(msg.id === undefined ? null : msg.id, -32600,
      "unsupported MCP-Protocol-Version " + version +
      "; supported: " + PROTOCOL_VERSIONS.join(", ")));

  // Notifications get 202 with no body (a bare number is an empty body).
  if (msg.id === undefined)
    return 202;
  if (typeof msg.method !== "string") {
    // A client response (result/error) is accepted; anything else is malformed.
    if (msg.result !== undefined || msg.error !== undefined)
      return 202;
    return jsonRpcHttp(400, rpcError(msg.id, -32600, "invalid request: missing method"));
  }
  return jsonRpcHttp(200, dispatch(msg));
}

function handleGet(req) {
  const blocked = originBlock(req);
  if (blocked)
    return blocked;
  if (req.path === "/")
    return { status: 200, headers: { "Content-Type": "text/html" }, body: statusPage() };
  if (req.path === "/mcp")
    return { status: 405, headers: { Allow: "POST, OPTIONS" },
             body: "this server does not offer SSE streams" };
  return { status: 404, body: "not found" };
}

function handleOptions(req) {
  const blocked = originBlock(req);
  if (blocked)
    return blocked;
  if (req.path !== "/mcp" && req.path !== "/")
    return { status: 404, body: "not found" };
  return 204;
}

function handleDelete(req) {
  const blocked = originBlock(req);
  if (blocked)
    return blocked;
  if (req.path !== "/mcp")
    return { status: 404, body: "not found" };
  return { status: 405, headers: { Allow: "POST, OPTIONS" },
           body: "this server does not use sessions" };
}

function statusPage() {
  return "<!doctype html><html><head><title>LibreSprite MCP Server</title></head>" +
    "<body><h1>LibreSprite MCP Server</h1>" +
    "<p>Running. Point your MCP client at <code>http://localhost:" + server.port + "/mcp</code>.</p>" +
    "<p>Tools: " + TOOLS.map((t) => t.name).join(", ") + "</p></body></html>";
}

// --- startup ---

const server = new HTTPServer({
  port: PORT,
  defaultHeaders: CORS_HEADERS,
  onPost: handlePost,
  onGet: handleGet,
  onOptions: handleOptions,
  onDelete: handleDelete,
});

const boundPort = server.start();
if (!boundPort) {
  console.log("MCP server could not start: " + server.error);
} else {
  console.log("MCP server listening on http://localhost:" + boundPort +
    "/mcp (read-only: " + READ_ONLY + ")");
}
