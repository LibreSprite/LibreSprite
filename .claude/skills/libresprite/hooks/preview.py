#!/usr/bin/env python3
import json
import re
import sys

event = json.load(sys.stdin)
command = event.get("tool_input", {}).get("command", "")
if "lsprite" not in command:
    sys.exit(0)
response = event.get("tool_response", {})
text = response.get("stdout", "") if isinstance(response, dict) else str(response)
paths = re.findall(r'"preview":\s*"([^"]+)"', text)
errors = re.findall(r'"ok":\s*false,\s*"error":\s*"([^"]+)"', text)
notes = []
if paths:
    notes.append("lsprite previews: " + ", ".join(paths) + ". Read the newest PNG with the Read tool and check it visually before the next edit.")
if errors:
    notes.append("lsprite failed: " + " | ".join(errors) + ". Fix the call; for command params run `lsprite command-info ID`.")
if notes:
    print(json.dumps({"hookSpecificOutput": {"hookEventName": "PostToolUse", "additionalContext": " ".join(notes)}}))
