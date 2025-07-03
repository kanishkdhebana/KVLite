#!/usr/bin/env bash
set -euo pipefail

# Resolve absolute path to directory containing this script
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Build path to the client binary: one level above SCRIPT_DIR
CLIENT="$SCRIPT_DIR/../client"

echo "Using client binary at: $CLIENT"

# Colors for readability
RED=$(tput setaf 1)
GREEN=$(tput setaf 2)
RESET=$(tput sgr0)

fail() {
  echo "${RED}FAILED:${RESET} $1"
  exit 1
}

pass() {
  echo "${GREEN}PASSED:${RESET} $1"
}

# SET command:-------------------------------------------------------------------------------------------

# 1) Set a new key
output=$($CLIENT set foo bar)
echo "$output"
echo "$output" | grep -q "status: NX" || fail "SET should return NX for new key"

# 2) Set same key again
output=$($CLIENT set foo baz)
echo "$output"
echo "$output" | grep -q "status: NX" || fail "SET should return NX when overwriting existing key"


# SET command:-------------------------------------------------------------------------------------------

# 3) Get the key
output=$($CLIENT get foo)
echo "$output"
echo "$output" | grep -q "status: OK" || fail "GET should return OK for existing key"
echo "$output" | grep -q "data: baz" || fail "GET should return updated value"

# 4) Get a missing key
output=$($CLIENT get missingkey)
echo "$output"
echo "$output" | grep -q "status: NX" || fail "GET should return NX for missing key"

# KEYS command:-------------------------------------------------------------------------------------------

output=$($CLIENT keys)
echo "$output"
echo "$output" | grep -q "status: OK" || fail "SET should return OK for missing key"


pass "All tests passed!"