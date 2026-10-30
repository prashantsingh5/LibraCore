#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "$0")/common.sh"
require_docker
echo 'Press Ctrl+C in the start_v3.sh terminal to stop its frontend/API processes.'
docker compose stop postgres
echo 'PostgreSQL stopped. Database volume and local media are retained.'
