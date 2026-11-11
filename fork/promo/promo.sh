#!/bin/sh
# Promo screenshots of the bookmarks bar, from a made-up home, as social
# cards: fork/promo/promo.sh OUTDIR. See fork/playbooks/promo-screenshots.md.
set -eu
HERE="$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)"
ROOT="$(CDPATH= cd -- "$HERE/../.." && pwd)"
OUT="$(realpath -m "${1:?usage: promo.sh OUTDIR}")"
RAW="$OUT/raw"

# Sharp (2x) shots of the real app, offscreen.
FOLIO_E2E_SEED="$HERE/seed.sh" FOLIO_E2E_ENV="QT_SCALE_FACTOR=2" \
  "$ROOT/tests/fork/e2e/run.sh" "$HERE/promo.steps" "$RAW"

# The Omarchy theme's colors for the cards; Tokyo Night when there's none.
COLORS="$HOME/.local/state/omarchy/current/theme/colors.toml"
color() { sed -n "s/^$1 *= *\"\\(#[0-9a-fA-F]*\\)\".*/\\1/p" "$COLORS" 2>/dev/null | head -1; }
BG="$(color background)"; BG="${BG:-#1a1b26}"
DARK="$(color darker_background)"; DARK="${DARK:-#0e0e14}"
ACCENT="$(color accent)"; ACCENT="${ACCENT:-#7aa2f7}"
FG="$(color bright_foreground)"; FG="${FG:-#c0caf5}"
MUTED="$(color foreground)"; MUTED="${MUTED:-#a9b1d6}"
FONT="$(fc-match 'monospace:bold' -f '%{file}')"
FONT_REGULAR="$(fc-match 'monospace' -f '%{file}')"

TITLE="A bookmarks bar for Files on Omarchy"
caption() {
  case "$1" in
    *hero*) echo "Ctrl+B bookmarks a folder at once. Name it your way." ;;
    *dropdown*) echo "Every project in one drop-down, like in your browser." ;;
    *move-invoices*) echo "Drag to reorder, into folders, or drop files on the bar." ;;
    *editor*) echo "A short name for a deep path. The path is the URL." ;;
  esac
}

# What each card shows of the window, in its logical pixels (the shots are
# 2x): "x y w h". Tight on the moment that matters, so the bar, the bubble
# and the menu still read in a phone's feed.
wide() {
  case "$1" in
    *hero*) echo "740 0 700 320" ;;
    *dropdown*) echo "0 0 600 280" ;;
    *move-invoices*) echo "0 0 560 250" ;;
    *editor*) echo "430 255 620 360" ;;
  esac
}
square() {
  case "$1" in
    *hero*) echo "900 0 540 420" ;;
    *dropdown*) echo "0 0 560 420" ;;
    *move-invoices*) echo "0 0 520 390" ;;
    *editor*) echo "430 250 620 440" ;;
  esac
}
# A part of a 2x shot.
cropped() {
  set -- "$1" $2 "$3"
  magick "$1" -crop "$(($4 * 2))x$(($5 * 2))+$(($2 * 2))+$(($3 * 2))" +repage "$6"
}

# A picture with rounded corners and a soft shadow, fitted into WxH.
framed() {
  magick "$1" -resize "$2" -alpha set \
    \( +clone -alpha transparent -fill white -draw "roundrectangle 0,0 %[fx:w-1],%[fx:h-1] 18,18" \) \
    -compose DstIn -composite -compose Over \
    \( +clone -background black -shadow 60x24+0+16 \) +swap -background none -layers merge +repage "$3"
}

# Words wrapped to a width.
words() { magick -background none -fill "$4" -font "$2" -pointsize "$3" -size "${5}x" caption:"$1" "$6"; }

mkdir -p "$OUT"
rm -f "$OUT"/*-16x9.png "$OUT"/*-1x1.png
T="$OUT/.tmp"; mkdir -p "$T"
for shot in "$RAW"/[0-9]*.png; do
  name="$(basename "$shot" .png | sed 's/^[0-9]*-//')"
  text="$(caption "$name")"
  [ -n "$text" ] || continue
  # 16:9 (X, LinkedIn, Mastodon).
  cropped "$shot" "$(wide "$name")" "$T/crop.png"
  framed "$T/crop.png" 1440x660 "$T/window.png"
  words "$TITLE" "$FONT" 56 "$FG" 1440 "$T/title.png"
  words "$text" "$FONT_REGULAR" 36 "$MUTED" 1440 "$T/caption.png"
  magick -size 1600x900 "radial-gradient:${BG}-${DARK}" \
    \( -size 1600x900 xc:none -fill "${ACCENT}22" -draw "circle 1500,-80 1500,520" \) -composite \
    "$T/title.png" -geometry +80+56 -composite \
    "$T/caption.png" -geometry +80+132 -composite \
    -fill "$ACCENT" -draw "rectangle 80,190 150,195" \
    \( "$T/window.png" \) -gravity north -geometry +0+222 -composite \
    -strip "$OUT/$name-16x9.png"
  # Square (Instagram, Bluesky).
  words "$TITLE" "$FONT" 54 "$FG" 940 "$T/title.png"
  words "$text" "$FONT_REGULAR" 36 "$MUTED" 940 "$T/caption.png"
  th=$(magick identify -format %h "$T/title.png")
  ch=$(magick identify -format %h "$T/caption.png")
  # The window takes what the words leave.
  cropped "$shot" "$(square "$name")" "$T/crop.png"
  framed "$T/crop.png" "940x$((1080 - 60 - th - 14 - ch - 40 - 50))" "$T/window.png"
  magick -size 1080x1080 "radial-gradient:${BG}-${DARK}" \
    "$T/title.png" -geometry +70+60 -composite \
    "$T/caption.png" -geometry +70+$((60 + th + 14)) -composite \
    \( "$T/window.png" \) -gravity north -geometry +0+$((60 + th + 14 + ch + 26)) -composite \
    -strip "$OUT/$name-1x1.png"
done
rm -rf "$T"
echo "Cards in $OUT (raw shots in $RAW)"
