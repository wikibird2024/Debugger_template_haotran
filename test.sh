#!/usr/bin/env bash
# Build and run every test against the fake board: no board and no display needed.
set -euo pipefail
cd "$(dirname "$0")"

cmake --preset debug >/dev/null
cmake --build --preset debug
ctest --preset debug "$@"
