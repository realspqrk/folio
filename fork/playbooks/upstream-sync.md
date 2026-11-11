# Playbook: keep the fork close to upstream (weekly)

Goal: `realspqrk/folio` = `omacom/folio` main + the bookmarks bar, rebased weekly, tested,
packaged; your install updated on your Go. Tool: `fork/bin/folio-sync` (on PATH as `folio-sync`).

## Shape (keep it this way)

- Fork `main`: pure mirror of `upstream/main`, fast-forward only, never committed on.
- Fork `spqrk` (GitHub default branch): `upstream/main` + **two** commits:
  1. `A bookmarks bar, as in a browser` — everything the bar needs, stock identity. New files
     (`src/Bookmark*.qml`, `BookmarksLayer.qml`, `FolderPicker.qml`, `src/bookmarks.*`,
     `tests/bookmarks_tests.cpp`) + small hooks in upstream files: `Main.qml` (8 lines),
     `PathBar.qml` (star), `FileMenu.qml` (2 entries), `FolderPane.qml`, `HelpOverlay.qml`,
     `Glyph.qml` (ribbon), `main.cpp`, `resources.qrc`, `backend.h`, `CMakeLists.txt`/`bin/test`
     (test target), `src/i18n/en.json` (the bar's strings).
  2. `Fork: folio-spqrk …` — identity (`main.cpp` names, `CMakeLists.txt` OUTPUT_NAME + `folio-e2e`,
     `src/plugins.cpp` plugin folder), `bin/build`/`bin/test` (e2e), `tests/fork/`, `fork/`
     (packaging in `fork/pkg/`, tooling, docs, promo).
- New bar code goes in the bar's own files; a hook in an upstream file stays a few lines.

## Weekly (timer, Monday 09:00: `folio-sync check`)

Ends in a notification and `folio-sync status`:

| state | means | do |
|---|---|---|
| `ready` | rebased clean, all tests green, package built | `folio-sync try` → `folio-sync publish` (Go) → `folio-sync install` |
| `conflict` | rebase stopped (files listed) | **Resolve** below |
| `red` | tests or package failed (names in status) | **Fix** below |
| `stopped` | upstream rewrote history | look; never force the fork's `main` |

`/folio-sync` (Claude skill) walks this: status → resolve/fix → review → your Go → publish → install.

## Resolve a conflict (in `~/Work/folio`, on a fresh branch)

1. `git fetch upstream && git fetch origin && git switch -c sync-$(date +%F) origin/spqrk`
   (a new branch: your `spqrk` and any local commits stay as they are).
2. `git rebase upstream/main`. rerere replays earlier resolutions; the en.json driver merges by key.
3. Per conflict: keep upstream's lines, re-add ours. Known shapes:
   - Hook next to a line upstream added (signals, Settings, margins, context properties): keep both.
   - Upstream renamed/moved a thing we hook (e.g. `pkgbuild/` → `share/`, Pin entry moved, Glyph
     lost fills): follow upstream; move our part into a fork-owned file rather than re-adding a hook.
   - `en.json` "both sides changed <key>": a real clash; take upstream's wording unless it's our key.
4. Follow upstream's new conventions in our strings/code (key labels, tooltips, `%Ln` plurals).
5. `bin/test` (upstream + `bookmarks_tests` + e2e). Upstream's catalog test lists missing strings:
   add them to `en.json` (sorted, its format). Keep exactly two commits (fix into the right one:
   `git commit --fixup=<sha>` then `GIT_SEQUENCE_EDITOR=: git rebase -i --autosquash upstream/main`).
6. Review: Codex Astra on `git range-diff origin/spqrk...HEAD`; fix until "fully done".
7. `folio-sync check --from "$PWD" sync-$(date +%F)` → must end `ready` (it builds, tests and
   packages that exact candidate). Then, on Go, `folio-sync publish` (atomic, leased) and
   `folio-sync install`. Never push the branch yourself: publish is the only way out.
8. `git switch spqrk && git reset --keep origin/spqrk` once you've checked `git log origin/spqrk..spqrk`
   is empty (otherwise rebase those commits onto origin/spqrk), then delete `sync-<date>`.

## Fix red tests

`folio-sync status` shows the failing names and the log tail (`state=error`: the stage that broke,
e.g. a fetch or a full disk). Reproduce on a fresh branch as in steps 1–2 above, fix in the commit it
belongs to (step 5), then steps 6–8.

## Recover

- Bad install: `folio-sync rollback` (reinstalls the package before).
- Candidate broken locally: `rm -rf ~/.local/share/folio-sync/repo` → `folio-sync check --force`
  re-clones and rebuilds (add the `--from` that `status` shows as `last_ready_from=`).
- Setup on a new machine: `~/Work/folio/fork/bin/folio-sync setup` (clone, driver, timer, PATH link).

## Before any publish

`git fetch upstream` right before: if `upstream/main` moved, rerun the same check first — with
the `--from` that `folio-sync status` shows as `last_ready_from=` (a plain `check` warns when you
forget it), or the repair drops out of the candidate.
Publishing, installing and GitHub settings need the user's Go.
