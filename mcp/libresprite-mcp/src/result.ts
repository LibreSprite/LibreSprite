import type { CallToolResult } from '@modelcontextprotocol/server';

/**
 * Operational failures come back as isError results rather than protocol
 * errors, so the model can read what went wrong and correct itself.
 */
export type ToolResult = CallToolResult;

export function json(data: unknown): ToolResult {
  return {
    content: [{ type: 'text', text: JSON.stringify(data, null, 2) }],
    structuredContent: data,
  };
}

export function text(message: string): ToolResult {
  return { content: [{ type: 'text', text: message }] };
}

export function failure(message: string): ToolResult {
  return { content: [{ type: 'text', text: message }], isError: true };
}

/** Wrap a handler so thrown errors become readable, model-recoverable results. */
export function guard<A>(fn: (args: A) => Promise<ToolResult>) {
  return async (args: A): Promise<ToolResult> => {
    try {
      return await fn(args);
    } catch (err) {
      return failure(err instanceof Error ? err.message : String(err));
    }
  };
}
