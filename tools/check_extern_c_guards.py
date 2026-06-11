#!/usr/bin/env python3
"""
check_extern_c_guards.py

Git commit checker for pico-sdk header files.
Ensures every .h file contains the standard C++ linkage guard:

    #ifdef __cplusplus
    extern "C" {
    #endif

Usage:
    # Check all headers under a directory (defaults to current dir)
    python check_extern_c_guards.py [path/to/pico-sdk]

    # Check only files staged for commit (pre-commit hook mode)
    python check_extern_c_guards.py --staged

    # Check files changed in a specific commit
    python check_extern_c_guards.py --commit HEAD

    # Check files changed in a range (useful for CI on PRs)
    python check_extern_c_guards.py --range origin/main..HEAD

    # Only check headers whose path matches a pattern
    python check_extern_c_guards.py --match "src/rp2_common/**/include/**"
    python check_extern_c_guards.py --match "**/include/pico/**"

    # Combine match and exclude for fine-grained control
    python check_extern_c_guards.py --match "**/include/**" --exclude "**/include/pico/config_autogen.h"

    # Skip files listed in an ignore list (reviewed, don't need guards)
    python check_extern_c_guards.py --ignore-list .extern_c_ignore

    # Exclude specific files or directories
    python check_extern_c_guards.py --exclude "test/**" --exclude "host/**"

    # Install as a git pre-commit hook
    python check_extern_c_guards.py --install-hook

Exit codes:
    0 - All checked headers contain the guard
    1 - One or more headers are missing the guard
"""

import argparse
import fnmatch
import os
import re
import subprocess
import sys
from pathlib import Path


# ---------------------------------------------------------------------------
# Pattern
# ---------------------------------------------------------------------------

# Matches the opening guard, tolerating varied whitespace and brace style.
# We look for:
#   #ifdef __cplusplus        (with optional leading/trailing whitespace)
#   extern "C" {              (with optional whitespace around tokens)
#   #endif                    (with optional leading/trailing whitespace)
#
# Lines between them may contain blank lines or comments.

GUARD_PATTERN = re.compile(
    r'#\s*ifdef\s+__cplusplus\s*\n'   # #ifdef __cplusplus
    r'(?:\s*(?://[^\n]*)?\n)*'         # optional blank / comment lines
    r'\s*extern\s+"C"\s*\{\s*\n'       # extern "C" {
    r'(?:\s*(?://[^\n]*)?\n)*'         # optional blank / comment lines
    r'\s*#\s*endif',                   # #endif
    re.MULTILINE,
)

# Files that are commonly exempt (pure macro / config headers with no symbols).
DEFAULT_EXCLUDES = [
    "**/pico/config_autogen.h",
    "**/generated/**",
]


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def git(*args: str) -> str:
    """Run a git command and return stripped stdout."""
    result = subprocess.run(
        ["git", *args],
        capture_output=True,
        text=True,
        check=True,
    )
    return result.stdout.strip()


def is_excluded(filepath: str, excludes: list[str]) -> bool:
    """Check whether a file matches any exclusion glob."""
    for pattern in excludes:
        if fnmatch.fnmatch(filepath, pattern):
            return True
    return False


def is_matched(filepath: str, matches: list[str]) -> bool:
    """Check whether a file matches at least one inclusion glob.

    Returns True if no match patterns are specified (i.e. everything matches
    by default), or if the filepath matches at least one of the patterns.
    """
    if not matches:
        return True
    return any(fnmatch.fnmatch(filepath, pat) for pat in matches)


def load_ignore_list(filepath: str) -> set[str]:
    """Load an ignore list file and return a set of normalised paths.

    The file format is one path per line, relative to the repo / scan root.
    Blank lines and lines starting with ``#`` are ignored.  Leading and
    trailing whitespace is stripped, and paths are normalised with forward
    slashes so that the file works cross-platform.
    """
    entries: set[str] = set()
    try:
        with open(filepath, encoding="utf-8") as fh:
            for raw_line in fh:
                line = raw_line.strip()
                if not line or line.startswith("#"):
                    continue
                # Normalise separators and remove trailing slashes
                normalised = line.replace("\\", "/").rstrip("/")
                entries.add(normalised)
    except FileNotFoundError:
        print(f"Warning: ignore list not found: {filepath}", file=sys.stderr)
    except Exception as exc:
        print(f"Warning: could not read ignore list {filepath}: {exc}", file=sys.stderr)
    return entries


def is_ignored(filepath: str, ignore_set: set[str]) -> bool:
    """Check whether a file's relative path appears in the ignore set.

    Compares using normalised forward-slash paths.  Also matches if any
    parent directory of the file is listed (so you can ignore a whole
    subtree by listing the directory).
    """
    if not ignore_set:
        return False
    normalised = filepath.replace("\\", "/").rstrip("/")
    if normalised in ignore_set:
        return True
    # Check parent directories so "src/foo" ignores everything under it
    parts = normalised.split("/")
    for i in range(1, len(parts)):
        if "/".join(parts[:i]) in ignore_set:
            return True
    return False


def has_extern_c_guard(content: str) -> bool:
    """Return True if the file content contains the extern C guard."""
    return GUARD_PATTERN.search(content) is not None


def is_header_file(path: str) -> bool:
    """Return True for .h files."""
    return path.endswith(".h")


def check_file(filepath: str) -> dict:
    """
    Check a single file. Returns a dict with:
        path     - the file path
        status   - 'ok', 'missing', 'skipped', or 'error'
        detail   - human-readable explanation (only for non-ok)
    """
    try:
        content = Path(filepath).read_text(encoding="utf-8", errors="replace")
    except FileNotFoundError:
        return {"path": filepath, "status": "error", "detail": "File not found"}
    except Exception as exc:
        return {"path": filepath, "status": "error", "detail": str(exc)}

    # Skip very short / empty stubs
    stripped = content.strip()
    if not stripped or stripped.count("\n") < 3:
        return {"path": filepath, "status": "skipped", "detail": "Too short / stub header"}

    if has_extern_c_guard(content):
        return {"path": filepath, "status": "ok"}

    return {
        "path": filepath,
        "status": "missing",
        "detail": 'Missing #ifdef __cplusplus / extern "C" guard',
    }


# ---------------------------------------------------------------------------
# File collection strategies
# ---------------------------------------------------------------------------

def collect_all_headers(root: str, excludes: list[str], matches: list[str] | None = None) -> list[str]:
    """Walk the tree and return all .h files."""
    headers = []
    for dirpath, _dirs, filenames in os.walk(root):
        for fname in filenames:
            if not is_header_file(fname):
                continue
            full = os.path.join(dirpath, fname)
            rel = os.path.relpath(full, root)
            if not is_excluded(rel, excludes) and is_matched(rel, matches or []):
                headers.append(full)
    headers.sort()
    return headers


def collect_staged(excludes: list[str], matches: list[str] | None = None) -> list[str]:
    """Return staged .h files (for pre-commit hook)."""
    raw = git("diff", "--cached", "--name-only", "--diff-filter=ACM")
    if not raw:
        return []
    return [
        f for f in raw.splitlines()
        if is_header_file(f) and not is_excluded(f, excludes) and is_matched(f, matches or [])
    ]


def collect_commit(ref: str, excludes: list[str], matches: list[str] | None = None) -> list[str]:
    """Return .h files changed in a specific commit."""
    raw = git("diff-tree", "--no-commit-id", "--name-only", "-r", "--diff-filter=ACM", ref)
    if not raw:
        return []
    return [
        f for f in raw.splitlines()
        if is_header_file(f) and not is_excluded(f, excludes) and is_matched(f, matches or [])
    ]


def collect_range(range_spec: str, excludes: list[str], matches: list[str] | None = None) -> list[str]:
    """Return .h files changed across a commit range."""
    raw = git("diff", "--name-only", "--diff-filter=ACM", range_spec)
    if not raw:
        return []
    return [
        f for f in raw.splitlines()
        if is_header_file(f) and not is_excluded(f, excludes) and is_matched(f, matches or [])
    ]


# ---------------------------------------------------------------------------
# Hook installer
# ---------------------------------------------------------------------------

HOOK_SCRIPT = """\
#!/bin/sh
# Auto-installed by check_extern_c_guards.py
exec python3 "{script}" --staged
"""


def install_hook():
    """Install this script as a git pre-commit hook."""
    try:
        hooks_dir = git("rev-parse", "--git-path", "hooks")
    except subprocess.CalledProcessError:
        print("Error: not inside a git repository.", file=sys.stderr)
        return 1

    hook_path = Path(hooks_dir) / "pre-commit"
    script_path = Path(__file__).resolve()

    if hook_path.exists():
        print(f"Warning: {hook_path} already exists.", file=sys.stderr)
        answer = input("Overwrite? [y/N] ").strip().lower()
        if answer != "y":
            print("Aborted.")
            return 1

    hook_path.write_text(HOOK_SCRIPT.format(script=script_path))
    hook_path.chmod(0o755)
    print(f"Installed pre-commit hook at {hook_path}")
    return 0


# ---------------------------------------------------------------------------
# Reporting
# ---------------------------------------------------------------------------

COLOUR_RED = "\033[91m"
COLOUR_GREEN = "\033[92m"
COLOUR_YELLOW = "\033[93m"
COLOUR_RESET = "\033[0m"
COLOUR_BOLD = "\033[1m"


def supports_colour() -> bool:
    return hasattr(sys.stdout, "isatty") and sys.stdout.isatty()


def report(results: list[dict], verbose: bool = False) -> int:
    """Print results and return the exit code (0 = pass, 1 = fail)."""
    colour = supports_colour()

    missing = [r for r in results if r["status"] == "missing"]
    errors = [r for r in results if r["status"] == "error"]
    skipped = [r for r in results if r["status"] == "skipped"]
    ok = [r for r in results if r["status"] == "ok"]

    if verbose:
        for r in ok:
            tag = f"{COLOUR_GREEN}PASS{COLOUR_RESET}" if colour else "PASS"
            print(f"  {tag}  {r['path']}")

    for r in skipped:
        tag = f"{COLOUR_YELLOW}SKIP{COLOUR_RESET}" if colour else "SKIP"
        print(f"  {tag}  {r['path']}  ({r['detail']})")

    for r in errors:
        tag = f"{COLOUR_RED}ERR {COLOUR_RESET}" if colour else "ERR "
        print(f"  {tag}  {r['path']}  ({r['detail']})")

    for r in missing:
        tag = f"{COLOUR_RED}FAIL{COLOUR_RESET}" if colour else "FAIL"
        print(f"  {tag}  {r['path']}")

    # Summary
    total = len(results)
    print()
    if colour:
        print(f"{COLOUR_BOLD}Summary:{COLOUR_RESET} ", end="")
    else:
        print("Summary: ", end="")

    print(
        f"{len(ok)} passed, {len(missing)} failed, "
        f"{len(skipped)} skipped, {len(errors)} errors "
        f"({total} headers checked)"
    )

    if missing:
        print()
        print("The following headers need an extern \"C\" guard added:")
        print()
        print("    #ifdef __cplusplus")
        print("    extern \"C\" {")
        print("    #endif")
        print()
        print("    // ... declarations ...")
        print()
        print("    #ifdef __cplusplus")
        print("    }")
        print("    #endif")
        print()
        return 1

    return 0


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------

def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Check that pico-sdk .h files contain extern \"C\" guards.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument(
        "path",
        nargs="?",
        default=".",
        help="Root directory to scan (default: current directory)",
    )
    mode.add_argument(
        "--staged",
        action="store_true",
        help="Check only git-staged .h files (pre-commit hook mode)",
    )
    mode.add_argument(
        "--commit",
        metavar="REF",
        help="Check .h files changed in a specific commit",
    )
    mode.add_argument(
        "--range",
        metavar="RANGE",
        help="Check .h files changed across a commit range (e.g. origin/main..HEAD)",
    )
    mode.add_argument(
        "--install-hook",
        action="store_true",
        help="Install this script as a git pre-commit hook",
    )
    parser.add_argument(
        "--match",
        action="append",
        default=[],
        help="Glob pattern to include — only matching paths are checked (may be repeated)",
    )
    parser.add_argument(
        "--exclude",
        action="append",
        default=[],
        help="Glob pattern to exclude (may be repeated)",
    )
    parser.add_argument(
        "--ignore-list",
        metavar="FILE",
        help=(
            "Path to a file listing headers to skip (one per line). "
            "These are files that have been reviewed and confirmed not to "
            "need extern C guards. Lines starting with # are comments."
        ),
    )
    parser.add_argument(
        "-v", "--verbose",
        action="store_true",
        help="Show passing files as well",
    )
    return parser


def main() -> int:
    parser = build_parser()
    args = parser.parse_args()

    if args.install_hook:
        return install_hook()

    excludes = DEFAULT_EXCLUDES + args.exclude
    matches = args.match  # empty list means "match everything"
    ignore_set = load_ignore_list(args.ignore_list) if args.ignore_list else set()

    # Collect files based on mode
    if args.staged:
        files = collect_staged(excludes, matches)
        label = "staged headers"
        rel_root = None
    elif args.commit:
        files = collect_commit(args.commit, excludes, matches)
        label = f"headers in commit {args.commit}"
        rel_root = None
    elif args.range:
        files = collect_range(args.range, excludes, matches)
        label = f"headers in range {args.range}"
        rel_root = None
    else:
        root = os.path.abspath(args.path)
        files = collect_all_headers(root, excludes, matches)
        label = f"headers under {root}"
        rel_root = root

    if matches:
        label += f" matching {matches}"

    # Partition into files to check vs files on the ignore list
    to_check = []
    ignored_results = []
    for f in files:
        rel = os.path.relpath(f, rel_root) if rel_root else f
        if is_ignored(rel, ignore_set):
            ignored_results.append({
                "path": f,
                "status": "skipped",
                "detail": "In ignore list",
            })
        else:
            to_check.append(f)

    if not files:
        print(f"No .h files found ({label}).")
        return 0

    print(f"Checking {len(to_check)} {label}"
          f"{f' ({len(ignored_results)} ignored)' if ignored_results else ''}...\n")

    results = [check_file(f) for f in to_check] + ignored_results
    return report(results, verbose=args.verbose)


if __name__ == "__main__":
    sys.exit(main())

