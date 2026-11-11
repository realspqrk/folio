#!/bin/sh
# The made-up home for promo shots, built from personas.md: an independent
# builder's code and business folders, an agency's clients, a tinkerer's
# configs. Generic names, generated content, nothing from this machine.
# Run by tests/fork/e2e/run.sh with DEMO set.
set -eu
cd "$DEMO"
app="Code/storefront"
mkdir -p "$app/app/models" "$app/app/controllers" "$app/app/views" "$app/bin" "$app/config" "$app/db" \
         "$app/lib" "$app/public" "$app/test" \
         Code/acme-portal Code/mobile-app Code/homelab Code/recipe-box Code/dotfiles \
         "Clients/Acme/2026/Q4 launch/Delivery" Clients/Globex Clients/Initech \
         Documents/Contracts "Documents/Finance/Invoices/2026" "Documents/Finance/Taxes/2026" \
         Downloads Music Pictures Videos Writing .config/hypr
cat > "$app/Gemfile" <<'RUBY'
source "https://rubygems.org"

gem "rails", "~> 8.1"
gem "sqlite3"
gem "puma"
gem "importmap-rails"
gem "turbo-rails"
gem "stimulus-rails"
gem "solid_queue"
gem "kamal", require: false
RUBY
cat > "$app/README.md" <<'MD'
# Storefront

The shop. `bin/dev` to run it, `bin/rails test` before you push.

Deploys with Kamal on every merge to main.
MD
cat > "$app/Dockerfile" <<'DOCKER'
FROM ruby:3.4-slim
WORKDIR /rails
COPY . .
RUN bundle install
CMD ["./bin/thrust", "./bin/rails", "server"]
DOCKER
printf 'require_relative "config/application"\n\nRails.application.load_tasks\n' > "$app/Rakefile"
# Folders with something in them, as a real repo's are.
for f in app/models/order.rb app/models/product.rb app/controllers/orders_controller.rb app/views/layout.html.erb \
         bin/dev bin/rails config/routes.rb config/database.yml config/deploy.yml db/schema.rb db/seeds.rb \
         lib/payments.rb public/robots.txt public/favicon.ico test/order_test.rb test/product_test.rb; do
  printf '# %s\n' "$f" > "$app/$f"
done
printf 'require_relative "config/environment"\n\nrun Rails.application\n' > "$app/config.ru"
printf '# Writing\n\nDrafts for the blog.\n' > Writing/drafts.md
cat > .config/folio-spqrk/bookmarks.json <<'JSON'
{"version":1,"bookmarks":[
 {"name":"Inbox","path":"~/Downloads"},
 {"name":"Projects","children":[
   {"name":"Acme portal","path":"~/Code/acme-portal"},
   {"name":"Mobile app","path":"~/Code/mobile-app"},
   {"name":"Side projects","children":[
     {"name":"Home server","path":"~/Code/homelab"},
     {"name":"Recipe box","path":"~/Code/recipe-box"}]}]},
 {"name":"Clients","children":[
   {"name":"Acme delivery","path":"~/Clients/Acme/2026/Q4 launch/Delivery"},
   {"name":"Globex","path":"~/Clients/Globex"},
   {"name":"Initech","path":"~/Clients/Initech"}]},
 {"name":"Writing","path":"~/Writing"},
 {"name":"Invoices","path":"~/Documents/Finance/Invoices/2026"},
 {"name":"Taxes 2026","path":"~/Documents/Finance/Taxes/2026"},
 {"name":"Dotfiles","path":"~/Code/dotfiles"},
 {"name":"Hyprland","path":"~/.config/hypr"},
 {"name":"Backup drive","path":"/run/media/backup"}
]}
JSON
# One fixed time for everything, so the cards come out the same on every run.
find "$DEMO" -mindepth 1 -not -path "$DEMO/.local/state/*" -exec touch -h -d "2026-11-11 09:41" {} +
