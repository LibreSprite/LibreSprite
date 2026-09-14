import { execFile } from 'node:child_process';
import { mkdtemp, rm, writeFile } from 'node:fs/promises';
import { tmpdir } from 'node:os';
import { join } from 'node:path';
import { promisify } from 'node:util';

import { ASSETS_ROOT, CONTAINER_ROOT } from './paths.js';
import { buildScript } from './templates.js';

const execFileAsync = promisify(execFile);

export const IMAGE = process.env.LIBRESPRITE_IMAGE ?? 'libresprite:mcp';
export const TIMEOUT_MS = Number(process.env.LIBRESPRITE_TIMEOUT_MS ?? 120_000);

/**
 * LS_MODE=dummy skips Xvfb entirely and relies on SDL's dummy video driver.
 * It is markedly faster per call but only works if the SDL build tolerates it
 * -- see the S1 spike. Default stays on the safe path.
 */
export const LS_MODE = process.env.LIBRESPRITE_LS_MODE ?? 'batch';

const SENTINEL_OPEN = '<<<LSMCP>>>';
const SENTINEL_CLOSE = '<<<END>>>';

export class LibreSpriteError extends Error {
  constructor(
    message: string,
    readonly stdout = '',
    readonly stderr = '',
  ) {
    super(message);
    this.name = 'LibreSpriteError';
  }
}

// ---------------------------------------------------------------------------
// Serialisation
//
// Each tool call runs its own throwaway container, so two calls touching the
// same .ase would race and the last writer would silently win. Chain calls per
// file instead. Keyed on the host path.
// ---------------------------------------------------------------------------

const locks = new Map<string, Promise<unknown>>();

export function withFileLock<T>(key: string, fn: () => Promise<T>): Promise<T> {
  const prev = locks.get(key) ?? Promise.resolve();
  const result = prev.then(fn, fn);

  // The chain tail must never reject, or the next waiter would inherit the
  // failure. Track that settled tail -- not `result` -- so the identity check
  // below actually matches and the map does not grow without bound.
  const tail = result.then(
    () => undefined,
    () => undefined,
  );
  locks.set(key, tail);
  void tail.then(() => {
    if (locks.get(key) === tail) locks.delete(key);
  });

  return result;
}

// ---------------------------------------------------------------------------
// Container invocation
// ---------------------------------------------------------------------------

interface RunOptions {
  /** Extra host dir to mount read-only at /scripts. */
  scriptDir?: string;
  /** Scripts have no business reaching the network (storage.fetch exists). */
  network?: 'none' | 'default';
}

function dockerArgs(opts: RunOptions): string[] {
  const args = ['run', '--rm', '--network', opts.network ?? 'none'];

  // Match the caller's uid so files land in the bind mount owned by them and
  // not by the image's uid 1000.
  if (typeof process.getuid === 'function' && typeof process.getgid === 'function') {
    args.push('--user', `${process.getuid()}:${process.getgid()}`);
  }

  args.push('-e', `LS_MODE=${LS_MODE}`);
  args.push('-v', `${ASSETS_ROOT}:${CONTAINER_ROOT}`);
  if (opts.scriptDir) {
    args.push('-v', `${opts.scriptDir}:/scripts:ro`);
  }
  args.push('-w', CONTAINER_ROOT, IMAGE);
  return args;
}

async function invoke(
  librespriteArgs: string[],
  opts: RunOptions = {},
): Promise<{ stdout: string; stderr: string }> {
  const argv = [...dockerArgs(opts), ...librespriteArgs];
  try {
    return await execFileAsync('docker', argv, {
      timeout: TIMEOUT_MS,
      maxBuffer: 64 * 1024 * 1024,
    });
  } catch (err) {
    const e = err as NodeJS.ErrnoException & { stdout?: string; stderr?: string };
    if (e.code === 'ENOENT') {
      throw new LibreSpriteError(
        'docker CLI not found on PATH. The MCP server drives LibreSprite through ephemeral containers.',
      );
    }
    const detail = (e.stderr ?? '').trim() || (e.stdout ?? '').trim() || e.message;

    // execFile reports a timeout kill as SIGTERM. Headless LibreSprite hangs
    // outright on the Transaction path (sprite.resize, sprite.loadPalette), so
    // name that rather than leaving a bare timeout.
    if ((e as { signal?: string }).signal === 'SIGTERM') {
      throw new LibreSpriteError(
        `LibreSprite timed out after ${TIMEOUT_MS}ms and was killed. Headless LibreSprite ` +
          `hangs on some operations (anything using its Transaction path). Output so far: ${detail}`,
        e.stdout ?? '',
        e.stderr ?? '',
      );
    }
    const exitCode = (e as { code?: number | string }).code;
    if (exitCode === 139 || (e as { signal?: string }).signal === 'SIGSEGV') {
      throw new LibreSpriteError(
        `LibreSprite crashed (SIGSEGV). This operation is not supported headless. Output: ${detail}`,
        e.stdout ?? '',
        e.stderr ?? '',
      );
    }
    throw new LibreSpriteError(
      `LibreSprite container failed: ${detail}`,
      e.stdout ?? '',
      e.stderr ?? '',
    );
  }
}

/**
 * Native CLI route: pure LibreSprite flags, no scripting. Preferred whenever
 * the flags in src/app/app_options.cpp cover the job -- it is faster and side
 * steps the ES5/Duktape constraints entirely.
 *
 * Callers must pass argv already in canonical order: LibreSprite processes
 * options and filenames positionally (src/app/app.cpp:214-639), so flags apply
 * to the files listed after them.
 */
export async function runNative(args: string[]): Promise<{ stdout: string; stderr: string }> {
  return invoke(['--batch', ...args]);
}

/**
 * Script route: renders an ES5 template with PARAMS baked in (this LibreSprite
 * has no --script-param), runs it, and parses the sentinel-wrapped JSON the
 * template prints on stdout.
 */
export async function runScript<T = unknown>(
  template: string,
  params: Record<string, unknown>,
): Promise<T> {
  const source = buildScript(template, params);
  const dir = await mkdtemp(join(tmpdir(), 'libresprite-mcp-'));
  try {
    await writeFile(join(dir, 'op.js'), source, 'utf8');
    const { stdout, stderr } = await invoke(['--batch', '--script', '/scripts/op.js'], {
      scriptDir: dir,
    });
    return parseSentinel<T>(stdout, stderr);
  } finally {
    await rm(dir, { recursive: true, force: true });
  }
}

/**
 * LibreSprite exits 0 even when a script throws, and mixes its own logging into
 * stdout, so the payload is fenced rather than parsed as whole-stream JSON.
 */
export function parseSentinel<T>(stdout: string, stderr = ''): T {
  const start = stdout.lastIndexOf(SENTINEL_OPEN);
  const end = stdout.lastIndexOf(SENTINEL_CLOSE);
  if (start === -1 || end === -1 || end < start) {
    const noise = (stdout.trim() || stderr.trim() || '(no output)').slice(-2000);
    throw new LibreSpriteError(
      `Script produced no result payload. LibreSprite output:\n${noise}`,
      stdout,
      stderr,
    );
  }

  const json = stdout.slice(start + SENTINEL_OPEN.length, end);
  let parsed: { ok: boolean; data?: T; error?: string };
  try {
    parsed = JSON.parse(json);
  } catch {
    throw new LibreSpriteError(`Result payload was not valid JSON: ${json.slice(0, 2000)}`);
  }

  if (!parsed.ok) {
    throw new LibreSpriteError(parsed.error ?? 'Script failed without an error message.');
  }
  return parsed.data as T;
}
