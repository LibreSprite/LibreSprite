#!/usr/bin/env python3
import json
import os
import shlex
import subprocess
import sys
from pathlib import Path

REPO = Path(os.environ.get("CLAUDE_PROJECT_DIR", Path(__file__).resolve().parents[4])).resolve()
BUILD = REPO / "build"


def check_js(path):
    proc = subprocess.run(["node", "--check", str(path)], capture_output=True, text=True)
    problems = [proc.stderr.strip()] if proc.returncode else []
    source = path.read_text(errors="replace")
    for banned in ("require(", "import ", "process."):
        if banned in source:
            problems.append(f"{path.name}: `{banned}` is not available in LibreSprite's QuickJS runtime")
    return problems


def check_cpp(path):
    db = BUILD / "compile_commands.json"
    if not db.exists():
        return []
    entries = json.loads(db.read_text())
    target = str(path.resolve())
    entry = next((e for e in entries if str(Path(e["directory"], e["file"]).resolve()) == target), None)
    if entry is None:
        return []
    args = entry.get("arguments") or shlex.split(entry["command"])
    cleaned = []
    skip = False
    for a in args:
        if skip:
            skip = False
            continue
        if a == "-o":
            skip = True
            continue
        if a == "-c":
            continue
        cleaned.append(a)
    proc = subprocess.run([*cleaned, "-fsyntax-only", "-Wall"], cwd=entry["directory"], capture_output=True, text=True)
    out = "\n".join(line for line in proc.stderr.splitlines() if str(path.name) in line or "error" in line)
    return [out[-3000:]] if proc.returncode or "warning:" in out else []


event = json.load(sys.stdin)
tool = event.get("tool_name", "")
inp = event.get("tool_input", {})
problems = []
if tool == "Bash":
    parts = shlex.split(inp.get("command", ""), posix=True) if "lsprite" in inp.get("command", "") else []
    for i, p in enumerate(parts):
        if p == "lsprite" and i + 3 < len(parts) and parts[i + 1] == "run":
            script = Path(parts[i + 3]).expanduser()
            if script.is_file():
                problems += check_js(script)
    if problems:
        print("\n".join(problems), file=sys.stderr)
        sys.exit(2)
    sys.exit(0)

file_path = inp.get("file_path")
if not file_path:
    sys.exit(0)
path = Path(file_path)
if not path.exists():
    sys.exit(0)
in_repo = REPO in path.resolve().parents
if path.suffix == ".js" and (in_repo or ".lsprite" in path.name or "lsprite" in str(path.parent)):
    problems += check_js(path)
elif path.suffix in (".cpp", ".cc") and in_repo:
    problems += check_cpp(path)
if problems:
    print("\n".join(problems), file=sys.stderr)
    sys.exit(2)
