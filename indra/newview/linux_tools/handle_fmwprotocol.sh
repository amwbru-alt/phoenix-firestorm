#!/bin/bash
set -e
url="${1:-}"
case "$url" in fmw://*) ;; *) exit 1 ;; esac
run_path="$(cd -- "$(dirname -- "$0")" && pwd)"
if command -v dbus-send >/dev/null && pgrep -f '/do-not-directly-run-firestorm-bin' >/dev/null; then
    if dbus-send --print-reply --reply-timeout=3000 --type=method_call \
        --dest=world.forme.ViewerAppAPIService /world/forme/ViewerAppAPI \
        world.forme.ViewerAppAPI.GoSLURL string:"$url" >/dev/null 2>&1; then
        exit 0
    fi
fi
exec "$run_path/../firestorm" -url "$url"
