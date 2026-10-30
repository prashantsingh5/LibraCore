#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "$0")/common.sh"
activate_project
export LLM_PROVIDER=mock RUN_LIVE_AI_TESTS=false
start_db
python -m libracore_api.database --create-test --test
cmake --build build-v2 --parallel "${BUILD_JOBS:-4}"
ctest --test-dir build-v2 --output-on-failure
./build-v2/bookmgmt_tests
./build-v2/bookmgmt_fault_tests
# Test fixtures reset ONLY libracore_test and migrate before each integration case.
export LIBRACORE_RUN_DB_TESTS=1
python -c 'import _libracore'
python -m pytest -ra
