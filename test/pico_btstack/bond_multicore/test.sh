#!/usr/bin/env bash
#
# Run the bond/reconnect test against the pico_multicore variant DUT, i.e. the
# regression test for https://github.com/raspberrypi/pico-sdk/issues/2725.
#
#   ./test.sh                 # flash this DUT and run every mode
#   ./test.sh --modes ctkd    # any ../bond/test.sh option is passed through
#
# This always flashes the firmware
#
# To reproduce the bug set PICO_MULTICORE_LOCKOUT_BEFORE_CORE1_STARTED_OVERRIDE=0
# The device will hit the assert on boot in debug builds
#

set -u -o pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BOND_TEST_SH="$SCRIPT_DIR/../bond/test.sh"
TARGET=pico_btstack_bond_multicore_test

if [ ! -x "$BOND_TEST_SH" ]; then
    echo "error: $BOND_TEST_SH not found or not executable" >&2
    exit 2
fi

# Mirrors default_uf2() in ../bond/test.sh, for this target.
default_uf2() {
    local sdk_root candidates c
    sdk_root="$(cd "$SCRIPT_DIR/../../.." && pwd)"
    candidates=(
        "${PICO_SDK_PATH:-$sdk_root}/build/test/pico_btstack/bond_multicore/$TARGET.uf2"
        "$sdk_root/build/test/pico_btstack/bond_multicore/$TARGET.uf2"
    )
    for c in "${candidates[@]}"; do
        [ -f "$c" ] && { echo "$c"; return 0; }
    done
    return 1
}

if ! UF2="$(default_uf2)"; then
    echo "error: no image found to flash. Build it with:" >&2
    echo "  cmake --build <build-dir> --target $TARGET" >&2
    echo "then pass --uf2 PATH if it is not under \$PICO_SDK_PATH/build" >&2
    exit 2
fi

exec "$BOND_TEST_SH" --uf2 "$UF2" "$@"
