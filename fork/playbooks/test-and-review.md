# Playbook: test and review the bookmarks bar

1. `bin/test`: upstream's `folio_tests`, the bar's `bookmarks_tests` (`tests/bookmarks_tests.cpp`), then
   `tests/fork/e2e/e2e.steps` (real app offscreen, real clicks/keys/drags, fails on any QML warning).
   Expect `Totals: … 0 failed` and `e2e: passed`. Never two e2e runs at once (shared `build/e2e`).
2. Screenshots for visual review (made-up home, real theme):
   ```sh
   tests/fork/e2e/run.sh tests/fork/e2e/walkthrough.steps /tmp/folio-shots
   # contact sheet of the top of each shot
   cd /tmp/folio-shots && for f in [0-9]*.png; do magick "$f" -crop 1280x280+0+0 +repage "c-$f"; done && magick c-*.png -append sheet.png
   ```
   Steps: `name=js`, or `click|rclick|mclick|press|move|release=X,Y|@js`, `key=Ctrl+B`, `type=text`,
   `resize=WxH`, `assert-x=js`; names ending `_` take no picture; helpers `chip(name)`,
   `at(name, fx)`, `names(list)`, `here()`.
3. Reviews (Codex via harness-subagent), one at a time, with `free -g` showing room first (three at
   once got killed for memory): code → Astra high; visual + workflow + promo privacy → Sol high;
   browser-fidelity UX → Astra medium. Gate: browser-like bookmarking adapted only to Omarchy style
   and local-file needs; data safety.
4. By hand (can't be simulated offscreen): drag a file from a pane onto the bar; middle-click opens
   a new window; Space preview floats (Hyprland rule).
