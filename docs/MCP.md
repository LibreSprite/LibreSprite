# Native MCP in LibreSprite

This fork embeds a **localhost MCP server** inside LibreSprite. Cursor talks to
a small stdio bridge, which forwards JSON-RPC to the running app.

```
Cursor  --stdio-->  tools/mcp_stdio_bridge.py  --HTTP-->  LibreSprite :64823
```

## 1. Build & run LibreSprite

```bash
cmake -G Ninja -DENABLE_MCP=ON ..
ninja libresprite
./bin/libresprite
```

## 2. Start the MCP server in the app

**File → MCP Server… → Start**

Or auto-start with:

```bash
LIBRESPRITE_START_MCP=1 ./bin/libresprite
```

This binds `127.0.0.1:64823` and writes:

`$XDG_CONFIG_HOME/libresprite/mcp-connection.json`  
(or `~/.config/libresprite/mcp-connection.json`)

## 3. Configure Cursor

Add to your Cursor MCP config (e.g. `~/.cursor/mcp.json`):

```json
{
  "mcpServers": {
    "libresprite": {
      "command": "python3",
      "args": [
        "/ABS/PATH/TO/LibreSprite-MCP/tools/mcp_stdio_bridge.py"
      ]
    }
  }
}
```

Optional: set `LIBRESPRITE_MCP_CONNECTION` to override the connection file path.

## 4. Tools

| Tool | Purpose |
|------|---------|
| `create_sprite` | New document (`width`, `height`, optional `color_mode`) |
| `get_sprite_info` | Active sprite metadata |
| `fill_sprite` | Fill active cel (`color`) |
| `draw_rect` | Filled rectangle |
| `set_pixels` | Sparse `{x,y,color}` list |
| `save_sprite` | Save active sprite (`path`) |
| `open_sprite` | Open file (`path`) |
| `run_script` | Eval JS (`code` or `path`) |

Colors accept `#RRGGBB`, `#RRGGBBAA`, `{r,g,b,a}`, or a raw integer.

## 5. Example prompts

- “Create a 32×32 RGB sprite and draw a red 8×8 square at (4,4).”
- “Set a checkerboard of black/white pixels, then save to `/tmp/hero.png`.”

## Security

- Listens on **loopback only**
- Requires a random **bearer token** from the connection file
- Stop anytime with **File → MCP Server… → Stop**
