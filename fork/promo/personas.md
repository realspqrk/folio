# Promo personas

Who the screenshots are for, and what their bookmarks bar would hold. The demo home
(`seed.sh`) is the overlap of the three: everyone should find their own day in it.

## 1. The independent builder (Omarchy's core: founder, indie dev, small studio)

Runs a small software business from one machine. Several codebases in flight, one of them the
product that pays the bills; writes and publishes; does the invoicing and taxes themselves; keeps
dotfiles and server configs close. Lives in the terminal, but still clicks through folders all day.

- Jumps between: the product's repo, a client project, the writing folder, this year's invoices.
- Pain today: the same five deep paths typed or dug for twenty times a day; pins are a flat list.
- What lands: **Projects ▾** with every codebase in one drop-down; **Invoices** and **Taxes 2026**
  next to the code; a short name for a deep path.

## 2. The agency owner / freelance designer

A handful of clients at once, each with briefs, contracts, assets and deliveries. Business folders
nested by client and year; brand assets reused across jobs.

- Jumps between: a client's current delivery, the shared brand kit, contracts, the photo shoot.
- Pain today: client trees are deep (`Clients/Acme/2026/Q4 launch/Delivery`), and pins can't group.
- What lands: **Clients ▾** as a folder of bookmarks; renaming `Q4 launch/Delivery` to
  "Acme delivery"; dragging a folder from the pane onto the bar.

## 3. The Linux power user / homelab tinkerer

Configures everything, runs a home server, keeps backups on a drive that isn't always plugged in.

- Jumps between: `~/.config/hypr`, dotfiles repo, the server's share, the backup drive, Downloads.
- Pain today: hidden config dirs and mounts are awkward to reach from a file manager.
- What lands: **Dotfiles** and **Hyprland** one click away; the **Backup drive** bookmark that
  stays (dimmed) when the drive is out; keyboard access (F6).

## The bar that serves all three

`Inbox` · `Projects ▾` · `Clients ▾` · `Writing` · `Invoices` · `Taxes 2026` · `Dotfiles` ·
`Backup drive` — eight chips, readable at 1440 px, mixing code, business and system folders.

Scenes:
1. **Hero** — in a Rails-style product repo (`~/Code/storefront`), Ctrl+B: "Bookmark added",
   named "Storefront", going into *Projects*. Code folders and text previews behind it.
2. **Drop-down** — *Projects ▾* open: Storefront, Acme portal, Mobile app, and *Side projects ▸*
   (Home server, Recipe box).
3. **Drag** — *Invoices* dragged next to *Inbox*: the money folders where the eyes are.
4. **Editor** — *Taxes 2026* → `~/Documents/Finance/Taxes/2026`: a short name for a deep path.
