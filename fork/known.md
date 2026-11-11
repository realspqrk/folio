# Known traps

## TOC
- [Qt Quick drag and drop](#qt-quick-drag-and-drop)
- [Qt Quick menus and popups](#qt-quick-menus-and-popups)
- [Offscreen e2e harness](#offscreen-e2e-harness)
- [Misc](#misc)

## Qt Quick drag and drop
- `MouseArea.drag.smoothed` (default true) makes the target lag the pointer by the distance moved
  before the drag started: drops land one chip off. Use `drag.smoothed: false`.
- Set `Drag.hotSpot` to the press point (`onPressed`), not the item centre, or drop zones are
  computed off-pointer.
- A dragged child of a `Row` draws under later siblings: raise the chip's `z` while dragging.
- A drop marker inside the target chip draws under the dragged copy (raised `z`): the bar's
  insertion mark lives on `Overlay.overlay`, above it. The copy is opaque, or the chips under it
  show through and the names run together.
- Never declare `property … position` on a `DropArea` (or any type with a `positionChanged(arg)` signal):
  the property's own `positionChanged()` hides it and handlers get no event ("Cannot read property 'y'").

## Qt Quick menus and popups
- A child that assigns a property its parent binds (`picker.folderId = …`) breaks the binding;
  emit a signal (`chosen(id)`) instead.
- `Menu.popup()` without a position uses `QCursor::pos()`; pass `popup(anchor, x, y)` from the
  mouse event (also needed offscreen, where the cursor never moves).
- Shortcuts outside the topmost popup that closes on Escape are blocked (`isBlockedByPopup`). Inside
  a Menu the keys are on the focused MenuItem: use `Keys.onPressed` on the entries, not a Shortcut.
- Qt `Menu`: Up with nothing highlighted does not wrap to the last entry; Down does start at the top.
- Opening any top-level `Menu` closes every other open `Menu` (`QQuickMenu::setVisible`, on purpose).
  A context menu over an open drop-down must be a plain `Popup` (the bar's `contextMenu`, with its
  own Up/Down/Return), not a `Menu`.
- A popup parented to a delegate dies when the `Repeater` rebuilds (every `bookmarks.items`
  change): hang drop-downs from the bar, not the chip.
- A right-click landing on a folder row in the same instant the pointer arrives races Qt's submenu
  hover timer: the submenu opens after the context menu and takes the keys. Hovering first (as hands
  do) is fine; e2e steps move, then right-click.
- After a drop-down is rebuilt under a resting pointer, Qt may open (and focus) the submenu now under
  it: one Escape closes that submenu first. That's menu behaviour, not lost focus.
- Dynamic menus: create with `createObject`, `menu.closed.connect(() => menu.destroy())`; map a
  right-click point to a lasting item *before* `menu.dismiss()` (the entry dies with the menu).

## Offscreen e2e harness
- `QQmlExpression` context properties: make a child `QQmlContext`, set properties, then build the
  expression; setting them on the window's own context afterwards isn't seen.
- The e2e gate ignores upstream's `QQuickImage: No thumbnail` (a file no thumbnailer takes, e.g.
  Ruby files without a text preview): an expected state, not a QML error.
- Offscreen (software rendering) doesn't clip `Shape` glyphs: the columns view's folder icons, scrolled
  left, paint over the sidebar and linger. Promo/e2e shots switch to grid first (Ctrl+1).
- e2e has its own CMake build dir (`build/e2e`, target `folio-e2e`).
- `FOLIO_E2E` inline steps split on `;`: put JS with `;` in a steps file.
- `QTest::keyClicks` has no `QWindow*` overload: loop `keyClick`.
- System drags (`Drag.Automatic`, files from a pane onto the bar) can't be simulated offscreen:
  check by hand.

## Misc
- Merge drivers come from attributes in the worktree *at that moment*: a driver or `.gitattributes`
  in the fork commit is missing while the feature commit replays. The en.json driver is set per
  clone (`.git/info/attributes`) and runs from `~/.local/share/folio-sync/json-union-merge`.
- `set -e` + `[ cond ] && cmd` as a block's last command aborts the script when cond is false: use `if`.
- Upstream (#56) made FolioDialog take Tab as a Shortcut: a field that needs Tab (path completion)
  claims it with `Keys.onShortcutOverride`, only when it has something to do.
- Upstream moved packaging to omarchy-pkgs (`pkgbuild/` gone, assets in `share/`) and hardcodes
  `/usr/share/folio/plugins`: the fork packages from `fork/pkg/` and installs plugins under
  `/usr/share/folio-spqrk/` (one-line hook in `src/plugins.cpp`).
- Upstream's Glyph lost fills (icons became SVG pictures): the filled star is `BookmarkStar.qml`.
- Upstream builds with CMake: `omarchy pkg add cmake` (needs sudo; from a prompt without a terminal there's no
  terminal, so `pkexec pacman -S --needed cmake`). No shims.
- `git checkout <rev> --` with nothing after `--` switches HEAD to <rev>; to take files use
  `git show <rev>:<path> > <path>` or `git checkout <rev> -- <paths>`.
- Never `open(p,'w').write(open(p).read())`: `'w'` truncates before the read runs, emptying the
  file. Fork files are untracked until the first commit; tar them to the scratchpad before scripted
  edits. (A lost QML file was recovered from a built binary: its qrc resources are zstd frames,
  `zstd -d` each and grep.)
- Don't run `tests/fork/e2e/run.sh` while a reviewer runs it too: both rebuild `build/e2e/folio-e2e`
  and steps fail at random (looks like flakiness).
- A `Repeater` on a JS array rebuilds every delegate on each change, dropping a click or drag in
  progress: the bar's chips sit on a `ListModel` synced by id (`syncChips`).
- `QFileSystemWatcher` can't watch a directory that doesn't exist yet: `mkpath` first.
- `git mv -k` silently does nothing for untracked paths.
- bash reads a running script from disk as it goes: editing `fork/bin/folio-sync` while a
  `check` runs can make it execute fragments after the edit. Wait for the check to end.
- The fork inherits upstream's `Sync translations` workflow, which runs on pushes to `main`, needs
  upstream's `MUSE_API_KEY` and would commit to the mirror: keep it disabled in the fork
  (`gh workflow disable "Sync translations" -R realspqrk/folio`).
