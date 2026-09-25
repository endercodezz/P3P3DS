#!/bin/sh
# Starts Yakumo. The tarball has it as ./yakumo next to Yakumo; the
# Flatpak runs it as its command, from /app/lib/yakumo.
here="$(dirname "$(readlink -f "$0")")"

# Recompiled code nests native calls deeply, and the main thread needs a
# 64 MiB stack. Yakumo raises the limit itself when it can; doing it
# here as well saves it a restart.
ulimit -s 65536 2>/dev/null || true

exec "$here/Yakumo" "$@"
