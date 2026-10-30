#!/usr/bin/env bash
set -euo pipefail
source "$(dirname "$0")/v3_common.sh"
v3_activate
export LLM_PROVIDER=mock RUN_LIVE_AI_TESTS=false LIBRACORE_RUN_DB_TESTS=0
v3_build
ctest --test-dir build-v3 --output-on-failure
./build-v3/bookmgmt_tests
./build-v3/bookmgmt_fault_tests
python -m unittest discover -s python/tests_v3/unit -v
python -m pytest python/tests python/tests_v3 -ra --cov=libracore_api.v3 --cov-report=term-missing --cov-report=html
python -m ruff check python/libracore_api/v3 python/tests_v3
python -m mypy python/libracore_api/v3 --follow-imports=silent --ignore-missing-imports
npm --prefix frontend run lint
npm --prefix frontend run typecheck
npm --prefix frontend run coverage
npm --prefix frontend run build
echo 'Offline verification finished. PostgreSQL, real E2E and Docker builds require the full script.'
