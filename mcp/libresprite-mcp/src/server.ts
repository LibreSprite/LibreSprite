#!/usr/bin/env node
/**
 * libresprite-mcp
 *
 * Exposes LibreSprite as an MCP server so an assistant can inspect, validate
 * and edit pixel art without a GUI. Every tool call runs a throwaway container:
 *
 *   docker run --rm --network none --user $UID:$GID \
 *     -v $LIBRESPRITE_ASSETS_ROOT:/work [-v <tmp>:/scripts:ro] \
 *     libresprite:mcp --batch [flags | --script /scripts/op.js]
 *
 * Environment:
 *   LIBRESPRITE_ASSETS_ROOT  host directory holding the art (default: cwd).
 *                            Nothing outside it is mounted or reachable.
 *   LIBRESPRITE_IMAGE        image tag (default: libresprite:mcp)
 *   LIBRESPRITE_LS_MODE      batch | dummy (default: batch)
 *   LIBRESPRITE_TIMEOUT_MS   per-call timeout (default: 120000)
 */
import { McpServer } from '@modelcontextprotocol/server';
import { StdioServerTransport } from '@modelcontextprotocol/server/stdio';

import { IMAGE, LS_MODE } from './docker.js';
import { ASSETS_ROOT } from './paths.js';
import { registerCreateTools } from './tools/create.js';
import { registerEditTools } from './tools/edit.js';
import { registerExportTools } from './tools/export.js';
import { registerInspectTools } from './tools/inspect.js';
import { registerPaletteTools } from './tools/palette.js';

const server = new McpServer({ name: 'libresprite', version: '0.1.0' });

registerCreateTools(server);
registerInspectTools(server);
registerExportTools(server);
registerPaletteTools(server);
registerEditTools(server);

async function main(): Promise<void> {
  // stdout carries the protocol, so any diagnostics must go to stderr.
  console.error(
    `libresprite-mcp: image=${IMAGE} mode=${LS_MODE} assetsRoot=${ASSETS_ROOT}`,
  );
  await server.connect(new StdioServerTransport());
}

main().catch((err: unknown) => {
  console.error('libresprite-mcp failed to start:', err);
  process.exit(1);
});
