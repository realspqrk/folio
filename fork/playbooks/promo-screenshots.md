# Playbook: promo screenshots of the bookmarks bar

Social cards (16:9 for X/LinkedIn/Mastodon, 1:1 for Instagram/Bluesky) of the real app, made
offscreen from a made-up home. Rerun after any visible change to the bar.

1. `fork/promo/promo.sh build/promo` — seeds `fork/promo/seed.sh` (generic folders, generated
   pictures, `bookmarks.json`), runs `fork/promo/promo.steps` at 2x through the e2e driver, then
   composes `build/promo/<scene>-16x9.png` and `-1x1.png` in the current Omarchy theme's colors.
   Raw 2x shots: `build/promo/raw/`.
2. Privacy (must hold before anything is published):
   - The run fails at `assert-private` if a drive or tailnet machine would show in the sidebar:
     unplug drives / stop tailscale, rerun.
   - Hermetic by `tests/fork/e2e/run.sh`: HOME, XDG config/data/cache are the made-up home, LANG
     is C, the Omarchy theme is the only thing taken from this machine (colors).
   - Look at every card: only made-up names (Clients, Acme Corp, Iceland 2026, …), `~` paths,
     no user name, hostname, real file or folder names, dates other than today's.
3. Who it's for: `fork/promo/personas.md` (independent builder, agency owner, Linux tinkerer); the
   seed and the scenes follow from it — revisit it before changing either.
   Scenes and captions live in `promo.steps` (`hero`, `dropdown`, `move-invoices`, `editor`: names
   without `_` are shots; grid first, Ctrl+1, see known.md); `caption()`, `wide()` and `square()` in
   `promo.sh` (each card's crop of the window, in logical pixels).
   Coordinates: `@chip(name)`, `@at(name, fx)`; drop-down rows are chip centre + 38 + 32·row.
   Crops stay tight on the moment (bar + bubble/menu/editor): check every card at 360 px wide
   (`magick card.png -resize 360x phone.png`) — the bar's labels and the dialog must still read.
4. Ship: copy the cards to `fork/promo/cards/` (committed; lossless PNG, ~350–600 KB each).
