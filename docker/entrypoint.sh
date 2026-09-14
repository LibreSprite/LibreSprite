#!/usr/bin/env bash
#
# LibreSprite container entrypoint.
#
# LS_MODE selects how much of the display stack is brought up before exec'ing
# LibreSprite. The binary always needs a video subsystem: src/she/sdl2/she.cpp
# calls SDL_Init(SDL_INIT_VIDEO) unconditionally, before any CLI flag is read,
# and aborts if it fails. So even `--batch` runs need one of these modes.
#
#   batch (default)  Xvfb only. For MCP / scripted runs.
#   gui              Xvfb + x11vnc + noVNC, reachable on 5900/6080.
#   dummy            No X server at all; SDL_VIDEODRIVER=dummy. Fastest, but
#                    only works if the SDL build tolerates the dummy driver.
#
set -euo pipefail

LS_MODE="${LS_MODE:-batch}"
RESOLUTION="${RESOLUTION:-1280x720x24}"
DISPLAY_NUM="${DISPLAY_NUM:-:99}"
VNC_PORT="${VNC_PORT:-5900}"
NOVNC_PORT="${NOVNC_PORT:-6080}"

# The image runs as uid 1000, but MCP calls pass --user $(id -u):$(id -g) so
# that files written into the bind mount are owned by the caller. That uid has
# no passwd entry and no home, and LibreSprite's ResourceFinder wants a
# writable HOME for its config. Fall back to a scratch dir when needed.
if [[ -z "${HOME:-}" || ! -w "${HOME:-}" ]]; then
    HOME="$(mktemp -d)"
    export HOME
fi

pids=()
cleanup() {
    if (( ${#pids[@]} )); then
        kill "${pids[@]}" 2>/dev/null || true
    fi
}
trap cleanup EXIT

start_xvfb() {
    Xvfb "${DISPLAY_NUM}" -screen 0 "${RESOLUTION}" \
        -ac -nolisten tcp +extension GLX +render -noreset &
    pids+=("$!")

    # Wait for the X socket instead of a fixed sleep. Saves ~1s per batch call,
    # which dominates the runtime of a short script.
    local socket="/tmp/.X11-unix/X${DISPLAY_NUM#:}"
    for _ in $(seq 1 100); do
        [[ -e "${socket}" ]] && return 0
        sleep 0.05
    done
    echo "entrypoint: Xvfb did not create ${socket} within 5s" >&2
    return 1
}

case "${LS_MODE}" in
    dummy)
        export SDL_VIDEODRIVER=dummy
        unset DISPLAY
        ;;
    batch)
        start_xvfb
        export DISPLAY="${DISPLAY_NUM}"
        ;;
    gui)
        start_xvfb
        export DISPLAY="${DISPLAY_NUM}"

        x11vnc -display "${DISPLAY_NUM}" \
            -forever -shared -nopw -quiet \
            -rfbport "${VNC_PORT}" \
            -listen 0.0.0.0 &
        pids+=("$!")

        websockify --web=/usr/share/novnc \
            0.0.0.0:"${NOVNC_PORT}" \
            localhost:"${VNC_PORT}" &
        pids+=("$!")
        ;;
    *)
        echo "entrypoint: unknown LS_MODE '${LS_MODE}' (expected batch|gui|dummy)" >&2
        exit 64
        ;;
esac

exec /opt/libresprite/libresprite "$@"
