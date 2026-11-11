#!/bin/sh
# Runs a steps file against the real app, offscreen, from a made-up home
# (the real Omarchy theme when there is one, made-up files and bookmarks):
#   tests/fork/e2e/run.sh STEPS OUTDIR
# e2e.steps is the end-to-end test (bin/test); walkthrough.steps takes the
# screenshots for visual review.
set -eu
ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/../../.." && pwd)"
STEPS="$(realpath "${1:?usage: run.sh STEPS OUTDIR}")"
OUT="$(realpath -m "${2:?usage: run.sh STEPS OUTDIR}")"
DEMO="$OUT/home"
rm -rf "$OUT" && mkdir -p "$OUT"
# The made-up home: FOLIO_E2E_SEED (a script run with DEMO set) or the
# default below.
mkdir -p "$DEMO/.local/state/omarchy" "$DEMO/.config/folio-spqrk"
# The Omarchy theme in use (colors only; nothing personal).
if [ -e "$HOME/.local/state/omarchy/current" ]; then ln -s "$HOME/.local/state/omarchy/current" "$DEMO/.local/state/omarchy/current"; fi
if [ -n "${FOLIO_E2E_SEED:-}" ]; then
  DEMO="$DEMO" sh "$FOLIO_E2E_SEED"
else
  mkdir -p "$DEMO/Documents/Taxes 2026" "$DEMO/Downloads" "$DEMO/Pictures/Lisbon 2026" \
           "$DEMO/Work/clients/acme" "$DEMO/Work/clients/globex" "$DEMO/Music" "$DEMO/Videos" \
           "$DEMO/.local/state/omarchy" "$DEMO/.config/folio-spqrk"
  mkdir -p "$DEMO/Work/Target" "$DEMO/Work/Target "
  echo plain > "$DEMO/Work/note.txt"; echo spaced > "$DEMO/Work/note.txt "
  echo hi > "$DEMO/Documents/invoice-0412.pdf"; echo x > "$DEMO/Work/notes.md"
  cat > "$DEMO/.config/folio-spqrk/bookmarks.json" <<'JSON'
{"version":1,"bookmarks":[
 {"name":"Inbox","path":"~/Downloads"},
 {"name":"Clients","children":[{"name":"ACME Corp","path":"~/Work/clients/acme"},{"name":"Globex","path":"~/Work/clients/globex"},{"name":"Archive","children":[{"name":"Old notes","path":"~/Work/notes.md"}]}]},
 {"name":"Taxes 26","path":"~/Documents/Taxes 2026"},
 {"name":"Invoice April","path":"~/Documents/invoice-0412.pdf"},
 {"name":"USB backup","path":"/run/media/backup"},
 {"name":"Fotos","path":"~/Pictures/Lisbon 2026"},
 {"name":"Music","path":"~/Music"},
 {"name":"Videos","path":"~/Videos"},
 {"name":"Globex quarterly","path":"~/Work/clients/globex"},
 {"name":"Notes","path":"~/Work/notes.md"},
 {"name":"Personal","children":[]}
]}
JSON
fi
# Its own build folder, so it can run beside bin/build and bin/test.
cmake -S "$ROOT" -B "$ROOT/build/e2e" -DCMAKE_BUILD_TYPE=Release >/dev/null
cmake --build "$ROOT/build/e2e" --target folio-e2e -j"$(nproc)" >/dev/null
cd "$ROOT/build/e2e"
# Nothing of this machine's: config, data and the thumbnail cache are the
# made-up home's, and the language is English.
# FOLIO_E2E_ENV adds more (QT_SCALE_FACTOR=2 for sharp promo shots).
# shellcheck disable=SC2086
env HOME="$DEMO" XDG_CONFIG_HOME="$DEMO/.config" XDG_DATA_HOME="$DEMO/.local/share" XDG_CACHE_HOME="$DEMO/.cache" \
  LANG=C.UTF-8 LC_ALL=C.UTF-8 QT_QPA_PLATFORM=offscreen QT_QPA_PLATFORMTHEME= ${FOLIO_E2E_ENV:-} \
  FOLIO_E2E_DIR="$OUT" FOLIO_E2E_STEPS="$STEPS" timeout 180 ./folio-e2e
