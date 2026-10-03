import { realpathSync } from 'node:fs';
import { dirname, isAbsolute, posix, relative, resolve, sep } from 'node:path';

/**
 * Everything the LLM can touch lives under ASSETS_ROOT on the host, mounted at
 * CONTAINER_ROOT inside the container. Nothing else is mounted, so a path that
 * escapes the root cannot resolve to anything -- but we reject it here anyway
 * so the failure is a clear message instead of a confusing container error.
 */
export const ASSETS_ROOT = resolve(
  process.env.LIBRESPRITE_ASSETS_ROOT ?? process.cwd(),
);

export const CONTAINER_ROOT = '/work';

export interface AssetPath {
  /** Absolute path on the host. */
  host: string;
  /** Absolute path as seen from inside the container. */
  container: string;
  /** Path relative to ASSETS_ROOT, always with forward slashes. */
  rel: string;
}

export class PathEscapeError extends Error {
  constructor(input: string) {
    super(
      `Path "${input}" resolves outside the assets root (${ASSETS_ROOT}). ` +
        `Use a path relative to the assets root.`,
    );
    this.name = 'PathEscapeError';
  }
}

function confine(candidate: string, input: string): string {
  const rel = relative(ASSETS_ROOT, candidate);
  if (rel === '' || rel.startsWith('..') || isAbsolute(rel)) {
    throw new PathEscapeError(input);
  }
  return rel;
}

/**
 * Resolve a caller-supplied path against ASSETS_ROOT and refuse anything that
 * escapes it, including via symlinks. Works for paths that do not exist yet
 * (outputs), in which case the nearest existing ancestor is the one checked.
 */
export function assetPath(input: string): AssetPath {
  if (input.trim() === '') {
    throw new Error('Path must not be empty.');
  }

  // An absolute input is only accepted if it is already inside the root.
  const host = isAbsolute(input) ? resolve(input) : resolve(ASSETS_ROOT, input);
  confine(host, input);

  // Re-check after following symlinks. Walk up to the nearest ancestor that
  // exists, since output paths legitimately do not.
  let probe = host;
  for (;;) {
    try {
      const real = realpathSync(probe);
      const suffix = relative(probe, host);
      confine(resolve(real, suffix), input);
      break;
    } catch (err) {
      if ((err as NodeJS.ErrnoException).code !== 'ENOENT') throw err;
      const parent = dirname(probe);
      if (parent === probe) break;
      probe = parent;
    }
  }

  const rel = relative(ASSETS_ROOT, host).split(sep).join(posix.sep);
  return { host, rel, container: posix.join(CONTAINER_ROOT, rel) };
}

/** Convenience for tools that take a list of paths. */
export function assetPaths(inputs: string[]): AssetPath[] {
  return inputs.map(assetPath);
}

/**
 * A scratch path inside the assets root. Needed because the container can only
 * see ASSETS_ROOT, so intermediate files for format conversion have to live
 * there. Callers must delete them.
 */
export function tempAsset(suffix: string): AssetPath {
  const name = `.mcp-tmp/${Date.now()}-${Math.random().toString(36).slice(2, 8)}${suffix}`;
  return assetPath(name);
}
