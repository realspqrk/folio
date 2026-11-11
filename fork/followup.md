# Follow-ups

## TOC
- [Features](#features)
- [Upstream](#upstream)

## Features
- While dragging a bookmark, hovering a folder chip or » could open it to drop deeper (browsers do); today a drag reaches the bar, the open drop-down and folder chips (into).
- Ctrl+click on a drop-down entry should open a new window as on the bar (MenuItem.triggered carries no modifiers); middle-click works.
- Drag the current folder from the path bar onto the bookmarks bar (browsers: drag the site icon) — needs a drag source in upstream's PathBar.
- Keep an open nested submenu chain open when the drop-down is rebuilt after a change (today the root stays open, submenus close).
- Ctrl+B / Ctrl+Shift+B / F6 while the path field is being edited (Ctrl+L), acting on the loaded folder; the star hides while editing.
- Bookmarks with an empty name (icon-only, as browsers allow) are refused today; only useful once glyphs tell bookmarks apart.
- New user-facing strings are English only: add them to `src/i18n/*.json` if the bar should follow Omarchy's language.
- Upstream's file chooser (`folio chooser`, the xdg portal, #132) runs in the fork's binary but isn't packaged: the portal stays stock folio's. Packaging it would need its own D-Bus name (`src/chooser.cpp`), and permanent chooser + bar tests (Ctrl+Return waits on a bookmark dialog; Replace's scrim covers the chips; the 2026-11-16 review's probes did this by hand).
- Optional `fork/bin/make-default`: personal `~/.config/mimeapps.list` `inode/directory=folio-spqrk.desktop` + Super+Shift+F rebind, for when the fork should replace stock Files.

## Upstream
- omacom/folio#44 (the bar) was closed: upstream keeps Pinned (now Ctrl+P, and right under Preview in the menu, omacom/folio#45). The bar lives on here as an additional feature, kept close to upstream.
