#!/usr/bin/env bash
# Push this folder to a fresh GitHub repo.
#
#   1. Make an EMPTY repo on github.com (no README, no .gitignore, no licence)
#   2. ./scripts/push.sh https://github.com/YOUR-NAME/oliverb.git
#
# After it finishes, open the repo's Actions tab — the plug-ins build themselves.
set -euo pipefail

if [ $# -ne 1 ]; then
  echo "usage: $0 <git-remote-url>"
  exit 1
fi

cd "$(dirname "$0")/.."

[ -d .git ] || git init
git add -A
git commit -m "OLIVERB: passive dub filter, tape echo and spring tank" || true
git branch -M main
git remote remove origin 2>/dev/null || true
git remote add origin "$1"
git push -u origin main

echo
echo "Done. Builds start automatically: ${1%.git}/actions"
