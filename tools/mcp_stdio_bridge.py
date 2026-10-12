#!/usr/bin/env python3
"""Stdio MCP bridge for LibreSprite's native localhost MCP server.

Cursor launches this process (stdio JSON-RPC). Each request is forwarded to
the running LibreSprite instance via HTTP POST /mcp with a bearer token.

Connection details are read from:
  $LIBRESPRITE_MCP_CONNECTION
  or $XDG_CONFIG_HOME/libresprite/mcp-connection.json
  or ~/.config/libresprite/mcp-connection.json
"""

from __future__ import annotations

import json
import os
import sys
import urllib.error
import urllib.request
from pathlib import Path
from typing import Any, Optional


def connection_path() -> Path:
    env = os.environ.get("LIBRESPRITE_MCP_CONNECTION")
    if env:
        return Path(env)
    xdg = os.environ.get("XDG_CONFIG_HOME")
    if xdg:
        return Path(xdg) / "libresprite" / "mcp-connection.json"
    return Path.home() / ".config" / "libresprite" / "mcp-connection.json"


def load_connection() -> dict[str, Any]:
    path = connection_path()
    if not path.is_file():
        raise FileNotFoundError(
            f"LibreSprite MCP connection file not found: {path}\n"
            "Start LibreSprite and choose File → MCP Server… → Start."
        )
    with path.open("r", encoding="utf-8") as f:
        return json.load(f)


def post_mcp(conn: dict[str, Any], message: dict[str, Any]) -> Optional[dict[str, Any]]:
    url = conn.get("url") or f"http://{conn.get('host', '127.0.0.1')}:{conn.get('port', 64823)}/mcp"
    token = conn["token"]
    data = json.dumps(message).encode("utf-8")
    req = urllib.request.Request(
        url,
        data=data,
        method="POST",
        headers={
            "Content-Type": "application/json",
            "Authorization": f"Bearer {token}",
            "Accept": "application/json",
        },
    )
    try:
        with urllib.request.urlopen(req, timeout=120) as resp:
            body = resp.read()
            if resp.status == 202 or not body:
                return None
            return json.loads(body.decode("utf-8"))
    except urllib.error.HTTPError as e:
        detail = e.read().decode("utf-8", errors="replace")
        return {
            "jsonrpc": "2.0",
            "id": message.get("id"),
            "error": {"code": -32000, "message": f"HTTP {e.code}: {detail}"},
        }
    except Exception as e:  # noqa: BLE001 - surface any bridge failure to the client
        return {
            "jsonrpc": "2.0",
            "id": message.get("id"),
            "error": {"code": -32000, "message": str(e)},
        }


def write_message(msg: dict[str, Any]) -> None:
    sys.stdout.write(json.dumps(msg, separators=(",", ":")) + "\n")
    sys.stdout.flush()


def main() -> int:
    try:
        conn = load_connection()
    except Exception as e:  # noqa: BLE001
        # Still speak MCP so the client sees a clear initialize failure.
        for line in sys.stdin:
            line = line.strip()
            if not line:
                continue
            try:
                msg = json.loads(line)
            except json.JSONDecodeError:
                continue
            if "id" in msg:
                write_message(
                    {
                        "jsonrpc": "2.0",
                        "id": msg.get("id"),
                        "error": {"code": -32000, "message": str(e)},
                    }
                )
        return 1

    for line in sys.stdin:
        line = line.strip()
        if not line:
            continue
        try:
            msg = json.loads(line)
        except json.JSONDecodeError:
            continue

        # Notifications have no id; still forward, ignore empty responses.
        if isinstance(msg, list):
            responses = []
            for item in msg:
                resp = post_mcp(conn, item)
                if resp is not None:
                    responses.append(resp)
            if responses:
                # Spec allows batch responses; emit one JSON value.
                write_message(responses)  # type: ignore[arg-type]
            continue

        resp = post_mcp(conn, msg)
        if resp is not None and "id" in msg:
            write_message(resp)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
