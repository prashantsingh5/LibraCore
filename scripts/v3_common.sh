#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")/.."
v3_activate() {
  [[ -x .venv/bin/python ]] || { echo 'Run ./scripts/setup_v3.sh first.' >&2; exit 1; }
  source .venv/bin/activate
  export LIBRACORE_BUILD_DIR=build-v3
  export PYTHONPATH="$PWD/build-v3/python:$PWD/python${PYTHONPATH:+:$PYTHONPATH}"
}
v3_build() {
  cmake -S . -B build-v3 -DLIBRACORE_V2=ON -DPython3_EXECUTABLE="$PWD/.venv/bin/python" -Dpybind11_DIR="$(python -m pybind11 --cmakedir)"
  cmake --build build-v3 --parallel "${BUILD_JOBS:-2}"
}
