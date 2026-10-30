#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")/.."
require_docker() {
  if ! command -v docker >/dev/null || ! docker compose version >/dev/null 2>&1 || ! docker info >/dev/null 2>&1; then
    echo 'Docker Compose/daemon unavailable. In Windows start Docker Desktop, enable Ubuntu WSL integration, then run this script inside WSL.' >&2
    exit 1
  fi
}
start_db() {
  require_docker
  docker compose up -d --wait --wait-timeout 120 postgres
}
activate_project() {
  if [[ ! -x .venv/bin/python ]]; then echo 'Run ./scripts/setup_v2.sh first.' >&2; exit 1; fi
  source .venv/bin/activate
  export PYTHONPATH="$PWD/build-v2/python:$PWD/python${PYTHONPATH:+:$PYTHONPATH}"
}
