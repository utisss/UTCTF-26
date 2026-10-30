#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")"

# Flatten includes into a single Dokploy-compatible compose file
# Uses relative paths so it works in any environment
docker compose config --no-interpolate --no-normalize 2>/dev/null |
  sed "s|$(pwd)/|./|g" |
  sed 's/published: "\([0-9]*\)"/published: \1/g' > compose.generated.yml

echo "Generated compose.generated.yml"
