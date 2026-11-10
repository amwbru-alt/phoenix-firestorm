#!/bin/bash
set -e
run_path="$(cd -- "$(dirname -- "$0")" && pwd)"
handler="${1:-$run_path/handle_fmwprotocol.sh}"
applications="${XDG_DATA_HOME:-$HOME/.local/share}/applications"
mkdir -p "$applications"
# Escape desktop-entry Exec characters without invoking a shell for the URL.
escaped_handler="$(printf '%s' "$handler" | sed 's/\\/\\\\/g; s/"/\\"/g; s/`/\\`/g; s/\$/\\$/g; s/%/%%/g')"
cat > "$applications/forme-world-url.desktop" <<EOF
[Desktop Entry]
Type=Application
Name=ForMe.World
NoDisplay=true
Exec="$escaped_handler" %u
Terminal=false
MimeType=x-scheme-handler/fmw;
EOF
if command -v update-desktop-database >/dev/null; then update-desktop-database "$applications"; fi
if command -v xdg-mime >/dev/null; then xdg-mime default forme-world-url.desktop x-scheme-handler/fmw; fi
