#!/usr/bin/env python3
"""
Script to run a matrix of tests via gdb via a running OpenOCD capturing UART output and checking for "PASSED" -
execution stops with a timeout or when GDB halts (e.g. breakpoint)

It requires 'grabserial'.

Iterates over path_segments and elf_filenames.

For every combination where <path_prefix>/<path_segment>/<path_postfix>/<elf_filename> exists:
  - Flashes it via OpenOCD (program + verify + reset + exit)
  - Captures UART output using grabserial
  - Saves to <output_dir>/<sanitized_path_segment>.<elf_filename>.out

At the end:
  - Checks that the last line of every generated .out file is exactly "PASSED" (after strip)
  - Prints success count / attempted count
  - Exits with 0 only if ALL attempted tests succeeded, else 1

Example usage:

$ tools/run_test_matrix.py \
    --path-segments \
        gcc_build \
        clang_build \
    --elf-filenames \
        custom_float_funcs_test_compiler.elf \
        custom_float_funcs_test_pico_dcp.elf \
        custom_float_funcs_test_pico.elf \
        custom_float_funcs_test_pico_vfp.elf \
    --path-postfix \
       test/pico_float_test \
    -- \
    out_dir

Which will run each of the following if they exist:
   gcc_build/test/pico_float_test/custom_float_funcs_test_compiler.elf
   gcc_build/test/pico_float_test/custom_float_funcs_test_pico_dcp.elf
   gcc_build/test/pico_float_test/custom_float_funcs_test_pico.elf
   gcc_build/test/pico_float_test/custom_float_funcs_test_pico_vfp.elf
   clang_build/test/pico_float_test/custom_float_funcs_test_compiler.elf
   clang_build/test/pico_float_test/custom_float_funcs_test_pico_dcp.elf
   clang_build/test/pico_float_test/custom_float_funcs_test_pico.elf
   clang_build/test/pico_float_test/custom_float_funcs_test_pico_vfp.elf
storing their UART output in:
   out_dir/gcc_build.custom_float_funcs_test_compiler.elf.out
   out_dir/gcc_build.custom_float_funcs_test_pico_dcp.elf.out
   out_dir/gcc_build.custom_float_funcs_test_pico.elf.out
   out_dir/gcc_build.custom_float_funcs_test_pico_vfp.elf.out
   out_dir/clang_build.custom_float_funcs_test_compiler.elf.out
   out_dir/clang_build.custom_float_funcs_test_pico_dcp.elf.out
   out_dir/clang_build.custom_float_funcs_test_pico.elf.out
   out_dir/clang_build.custom_float_funcs_test_pico_vfp.elf.out

This would run the same tests:
(but store the output in e.g out_dir/gcc_build.pico_float_test.custom_float_funcs_test_compiler.elf.out )

$ tools/run_test_matrix.py \
    --path-segments \
        gcc_build \
        clang_build \
    --elf-filenames \
        pico_float_test/custom_float_funcs_test_compiler.elf \
        pico_float_test/custom_float_funcs_test_pico_dcp.elf \
        pico_float_test/custom_float_funcs_test_pico.elf \
        pico_float_test/custom_float_funcs_test_pico_vfp.elf \
    --path-postfix \
       test \
    -- \
    out_dir
"""

import argparse
import os
import subprocess
import sys
import time
import signal

def main():
    parser = argparse.ArgumentParser(description="RP2xxx ELF test runner with OpenOCD + grabserial UART capture")
    parser.add_argument("output_dir", help="Directory where *.out files will be written (will be created if missing)")
    parser.add_argument("--path-segments", nargs="+", required=True,
                        help="Space-separated list of path segments; e.g. foo, bar, foo/bar/humbug")
    parser.add_argument("--path-prefix", default=".",
                        help="Path prefix")
    parser.add_argument("--path-postfix", default="",
                        help="Path postfix")
    parser.add_argument("--gdb", default="arm-none-eabi-gdb",
                        help="gdb executable")
    parser.add_argument("--gdb-timeout", type=int, default=60,
                        help="gdb timeout for executable if it doesn't finish")
    parser.add_argument("--openocd-server", default="localhost:3333",
                        help="openocd server address:port"),
    parser.add_argument("--elf-filenames", nargs="+", required=True,
                        help="Space-separated list of ELF filenames (including .elf), e.g. test1.elf test2.elf")
    parser.add_argument("--serial-port", default="/dev/ttyACM0", help="UART device (default: /dev/ttyACM0)")
    parser.add_argument("--baud", type=int, default=115200, help="UART baud rate (default: 115200)")

    args = parser.parse_args()

    os.makedirs(args.output_dir, exist_ok=True)

    attempted = 0
    succeeded = 0

    for seg in args.path_segments:
        for elf in args.elf_filenames:
            full_path = os.path.join(args.path_prefix, seg, args.path_postfix, elf)
            if not os.path.isfile(full_path):
                print(f"\n=== Skipping missing ELF {full_path} ===")
                continue

            attempted += 1
            sanitized_out_file = f"{seg}.{elf}.out".replace("/",".")
            outfile = os.path.join(args.output_dir, sanitized_out_file)

            print(f"\n=== Processing {seg}/{elf} ===")
            print(f"   ELF: {full_path}")
            print(f"   Out: {outfile}")

            # ====================== START GRABSERIAL EARLY FOR FULL UART CAPTURE ======================
            print(f"   Starting grabserial capture early on {args.serial_port} @ {args.baud} baud...")

            logger_proc = None
            try:
                logger_proc = subprocess.Popen(
                    [
                        "grabserial",
                        "-d", args.serial_port,
                        "-b", str(args.baud),
                        "-o", outfile,          # output file
                        "--quiet",             # suppress extra messages
                    ],
                    stdin=subprocess.DEVNULL,
                    stdout=subprocess.DEVNULL,
                    stderr=subprocess.PIPE,
                    preexec_fn=os.setsid,
                )
                print(f"   grabserial started (PID {logger_proc.pid}) — capturing from now on")

                # Give grabserial a moment to open the port and settle
                time.sleep(0.5)

                # ====================== FLASH + START VIA GDB ======================
                print("   Flashing and running via GDB (breakpoint signals test end)...")

                gdb_cmd = [
                    args.gdb, "--batch", "--nx", full_path,
                    "-ex", f"target extended-remote {args.openocd_server}",
                    "-ex", "monitor reset init",
                    "-ex", "load",
                    "-ex", "monitor verify",   # optional
                    "-ex", "continue",
                ]

                gdb_result = subprocess.run(
                    gdb_cmd,
                    check=False,
                    timeout=args.gdb_timeout,
                    capture_output=True,
                    text=True, encoding="utf-8", errors="replace"
                )

                # Show GDB output always (super useful for diagnosing)
                combined_output = gdb_result.stdout + "\n" + gdb_result.stderr
                if combined_output.strip():
                    print("   GDB output:")
                    print("   ────────────────────────────────────────────────")
                    print(combined_output.strip())
                    print("   ────────────────────────────────────────────────")

                if "breakpoint" in combined_output:
                    print("   ✓ Breakpoint hit → test finished normally")
                else:
                    print("   Note: No breakpoint detected in GDB output")

                # Optional: tiny extra wait for any trailing UART bytes after breakpoint
                time.sleep(0.5)

            except FileNotFoundError:
                print("   ERROR: grabserial or gdb not found in PATH")
                sys.exit(1)
            except subprocess.TimeoutExpired:
                print("   GDB timed out")
                # fall through to kill grabserial
            finally:
                # ====================== CLEANLY STOP GRABSERIAL ======================
                if logger_proc is not None:
                    try:
                        print("   Stopping grabserial...")
                        os.killpg(os.getpgid(logger_proc.pid), signal.SIGTERM)
                        logger_proc.wait(timeout=5)
                        print("   grabserial stopped cleanly")
                    except ProcessLookupError:
                        print("   grabserial already exited")
                    except Exception as e:
                        print(f"   Error stopping grabserial: {e}")
                        # forceful kill if needed
                        try:
                            os.killpg(os.getpgid(logger_proc.pid), signal.SIGKILL)
                        except:
                            pass

            # ====================== CHECK WE CAPTURED SOME OUTPUT ======================
            if not os.path.isfile(outfile) or os.path.getsize(outfile) == 0:
                print("   No output captured")
                continue

            # ====================== PRINT THE CAPTURED OUTPUT TO CONSOLE ======================
            print("\n" + "="*60)
            print(f"Full captured UART output from {seg}/{elf}:")
            print("="*60)

            try:
                with open(outfile, "r", encoding="utf-8", errors="replace") as f:
                    captured_content = f.read()

                if captured_content.strip():
                    print(captured_content.rstrip())   # rstrip to avoid trailing newlines
                else:
                    print("(No output captured)")

            except FileNotFoundError:
                print("Output file not found — capture may have failed")
            except Exception as e:
                print(f"Error reading output file: {e}")

            print("="*60 + "\n")
            # ====================== CHECK LAST LINE ======================
            try:
                with open(outfile, "r", encoding="utf-8", errors="ignore") as f:
                    lines = f.readlines()
                    last_line = lines[-1].strip() if lines else ""
                if last_line == "PASSED":
                    succeeded += 1
                    print("   ✓ PASSED")
                else:
                    print(f"   ✗ FAILED (last line: '{last_line}')")
            except Exception as e:
                print(f"   Failed to read output file: {e}")

    # ====================== FINAL SUMMARY ======================
    print("\n" + "="*60)
    print(f"TEST SUMMARY: {succeeded}/{attempted} succeeded")
    print("="*60)

    if attempted == 0:
        print("No ELFs were found to test.")
    elif succeeded == attempted:
        print("All tests passed!")
    else:
        print("Some tests failed.")
        sys.exit(1)


if __name__ == "__main__":
    main()