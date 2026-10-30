#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "$0")/common.sh"
activate_project
start_db
AI_ENABLED=false python -m libracore_api.database
AI_ENABLED=false python -m libracore_api.seed
python -m libracore_api.run
