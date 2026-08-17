#!/usr/bin/env bash
#
# Run the bond/reconnect regression test in every mode and report a single
# verdict. Prints "PASSED" and exits 0 only if all modes pass.
#
#   ./test.sh
#   ./test.sh --flash
#   ./test.sh --addr 2C:CF:67:BE:08:05 --uf2 path/to/pico_btstack_bond_test.uf2
#
# The DUT must already be running the bond test firmware unless --flash/--uf2
# is given. Flashing is opt-in because it erases the bond state, and re-running
# is useful

set -u -o pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TEST_PY="$SCRIPT_DIR/bond_reconnect_test.py"

ADDR="${BOND_TEST_ADDR:-}"
NAME="${BOND_TEST_NAME:-}"
UF2=""
DO_FLASH=0
TRANSPORT="usb:0"
MODES="ctkd classic le"
PYTHON="${BOND_TEST_PYTHON:-}"
VERBOSE=""

# Seconds to wait for a USB re-enumeration. WSL2 remaps the device to Linux a
# good few seconds after it changes mode, so this needs to be generous.
USB_WAIT=30
# Let the DUT's radio settle between modes.
BETWEEN_MODES=3

usage() {
    sed -n '2,10p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'
    cat <<'EOF'

options:
  --addr ADDR        DUT Bluetooth address (or set BOND_TEST_ADDR)
  --name NAME        find the DUT by advertised name (default: PicoBondTest)
  --flash            erase and flash the DUT before testing (finds the image
                     built by the SDK; opt-in because it erases bond state)
  --uf2 PATH         as --flash, but use this image
  --transport SPEC   Bumble transport for the peer radio (default: usb:0)
  --modes "a b c"    modes to run (default: "ctkd classic le")
  --python PATH      interpreter with bumble installed (default: ~/venv/bin/python)
  -v                 pass -v through to the test
EOF
}

while [ $# -gt 0 ]; do
    case "$1" in
        --addr)      ADDR="$2"; shift 2 ;;
        --name)      NAME="$2"; shift 2 ;;
        --uf2)       UF2="$2"; DO_FLASH=1; shift 2 ;;
        --flash)     DO_FLASH=1; shift ;;
        --transport) TRANSPORT="$2"; shift 2 ;;
        --modes)     MODES="$2"; shift 2 ;;
        --python)    PYTHON="$2"; shift 2 ;;
        -v)          VERBOSE="-v"; shift ;;
        -h|--help)   usage; exit 0 ;;
        *)           echo "unknown option: $1" >&2; usage >&2; exit 2 ;;
    esac
done

if [ -z "$ADDR" ] && [ -z "$NAME" ]; then
    # Default to discovery: the DUT advertises this name, so no per-board
    # address is needed
    NAME="PicoBondTest"
fi
if [ -n "$ADDR" ]; then
    TARGET_ARGS=(--addr "$ADDR"); TARGET_DESC="$ADDR"
else
    TARGET_ARGS=(--name "$NAME"); TARGET_DESC="name '$NAME'"
fi

# Where the SDK build puts the DUT image
default_uf2() {
    local sdk_root candidates c
    sdk_root="$(cd "$SCRIPT_DIR/../../.." && pwd)"
    candidates=(
        "${PICO_SDK_PATH:-$sdk_root}/build/test/pico_btstack/bond/pico_btstack_bond_test.uf2"
        "$sdk_root/build/test/pico_btstack/bond/pico_btstack_bond_test.uf2"
    )
    for c in "${candidates[@]}"; do
        [ -f "$c" ] && { echo "$c"; return 0; }
    done
    return 1
}

if [ "$DO_FLASH" -eq 1 ] && [ -z "$UF2" ]; then
    if ! UF2="$(default_uf2)"; then
        echo "error: no image found to flash. Build it with:" >&2
        echo "  cmake --build <build-dir> --target pico_btstack_bond_test" >&2
        echo "then pass --uf2 PATH if it is not under \$PICO_SDK_PATH/build" >&2
        exit 2
    fi
    echo "${C_DIM:-}using $UF2${C_OFF:-}"
fi

if [ -z "$PYTHON" ]; then
    if [ -x "$HOME/venv/bin/python" ]; then
        PYTHON="$HOME/venv/bin/python"
    else
        PYTHON="python3"
    fi
fi

if ! "$PYTHON" -c "import bumble" 2>/dev/null; then
    echo "error: bumble not importable with $PYTHON" >&2
    echo "       try: $PYTHON -m pip install bumble" >&2
    exit 2
fi

if [ ! -f "$TEST_PY" ]; then
    echo "error: $TEST_PY not found" >&2
    exit 2
fi

if tty -s <&1 2>/dev/null; then
    C_OK=$'\033[32m'; C_BAD=$'\033[31m'; C_DIM=$'\033[2m'; C_OFF=$'\033[0m'
else
    C_OK=""; C_BAD=""; C_DIM=""; C_OFF=""
fi

wait_for_bootsel() {
    local i
    for ((i = 0; i < USB_WAIT; i++)); do
        picotool info >/dev/null 2>&1 && return 0
        sleep 1
    done
    return 1
}

wait_for_app() {
    # Device has left BOOTSEL and is running once picotool can no longer see a
    # BOOTSEL device. Give it a moment more to bring the radio up.
    local i
    for ((i = 0; i < USB_WAIT; i++)); do
        picotool info >/dev/null 2>&1 || { sleep 3; return 0; }
        sleep 1
    done
    return 1
}

flash_dut() {
    local uf2="$1"
    [ -f "$uf2" ] || { echo "error: no such file: $uf2" >&2; return 1; }

    echo "${C_DIM}flashing $uf2${C_OFF}"
    if ! picotool info >/dev/null 2>&1; then
        # Running the application: ask it to reboot into BOOTSEL. This fails
        # silently if the firmware has asserted
        picotool reboot -f -u >/dev/null 2>&1 || true
    fi
    if ! wait_for_bootsel; then
        echo "error: device did not appear in BOOTSEL." >&2
        echo "       If the firmware asserted it cannot service the reset" >&2
        echo "       interface -- power-cycle or hold BOOTSEL and retry." >&2
        return 1
    fi
    picotool erase >/dev/null 2>&1 || { echo "error: erase failed" >&2; return 1; }
    picotool load -x "$uf2" >/dev/null 2>&1 || { echo "error: load failed" >&2; return 1; }
    wait_for_app || true
    return 0
}

run_mode() {
    # Exit 2 is a harness/setup problem rather than a verdict, so retry once.
    local mode="$1" rc attempt
    for attempt in 1 2; do
        "$PYTHON" "$TEST_PY" "${TARGET_ARGS[@]}" --transport "$TRANSPORT" \
            --mode "$mode" $VERBOSE
        rc=$?
        [ "$rc" -ne 2 ] && return "$rc"
        if [ "$attempt" -eq 1 ]; then
            echo "${C_DIM}  setup error, retrying $mode once...${C_OFF}"
            sleep "$BETWEEN_MODES"
        fi
    done
    return "$rc"
}

if [ "$DO_FLASH" -eq 1 ]; then
    flash_dut "$UF2" || exit 2
fi

echo "DUT $TARGET_DESC via $TRANSPORT"
echo

declare -a NAMES=() RESULTS=()
overall=0
setup_error=0

for mode in $MODES; do
    echo "${C_DIM}=== mode: $mode ===${C_OFF}"
    run_mode "$mode"
    rc=$?
    case "$rc" in
        0) RESULTS+=("${C_OK}pass${C_OFF}") ;;
        1) RESULTS+=("${C_BAD}FAIL${C_OFF}"); overall=1 ;;
        *) RESULTS+=("${C_BAD}error${C_OFF}"); overall=1; setup_error=1 ;;
    esac
    NAMES+=("$mode")
    echo
    sleep "$BETWEEN_MODES"
done

echo "-------- summary --------"
for i in "${!NAMES[@]}"; do
    printf '  %-8s %s\n' "${NAMES[$i]}" "${RESULTS[$i]}"
done
echo

if [ "$overall" -eq 0 ]; then
    echo "${C_OK}PASSED${C_OFF}"
    exit 0
fi

if [ "$setup_error" -eq 1 ]; then
    echo "${C_BAD}FAILED${C_OFF} (setup error -- is the DUT running? check its"
    echo "console for an assertion"
else
    echo "${C_BAD}FAILED${C_OFF}"
fi
exit 1
