#!/usr/bin/env python3
"""
Print the tree of linker script INCLUDE files starting from a base memmap .ld file,
and where they are from.

Pass extra folders (e.g. those set by pico_add_linker_script_override_path) with -L.
Files from those folders are highlighted in green.

Use -o FILE to also write the fully expanded linker script to FILE.

Use --sdk-root and --platform when the memmap lives outside the SDK tree.

Examples:

    ./tools/memmap_include_tree.py src/rp2350/pico_platform/memmap_copy_to_ram.ld

    ./tools/memmap_include_tree.py src/rp2040/pico_platform/memmap_default.ld \
        -L test/kitchen_sink/kitchen_sink_ram_section_scripts \
        -L test/kitchen_sink/kitchen_sink_extra_end_scripts

    ./tools/memmap_include_tree.py /path/to/my_memmap.ld --sdk-root /path/to/pico-sdk \\
        --platform rp2040
"""

from __future__ import annotations

import argparse
import os
import re
import sys
from pathlib import Path
from typing import Iterable

# (included_path, children, -L label from CLI if resolved via a user -L path)
ChildRow = tuple[Path, list["ChildRow"], str | None]


# Strip C-style block comments (non-greedy); good enough for SDK linker fragments.
_BLOCK_COMMENT_RE = re.compile(r"/\*.*?\*/", re.DOTALL)
# INCLUDE "file" or INCLUDE file
_INCLUDE_RE = re.compile(
    r'^\s*INCLUDE\s+"(?P<quoted>[^"]+)"\s*(?:$|;)'
    r'|^\s*INCLUDE\s+(?P<bare>[^";\s]+)\s*(?:$|;)',
    re.MULTILINE,
)

# CMake generates this in the binary dir; skip it so the tree is useful without a build.
IGNORED_INCLUDE_NAMES = frozenset({"pico_flash_region.ld", "pico_psram_region.ld"})

# Shared CLI helpers (argparse help strings, colors, doc folding) for memmap_annotate.py
CLI_HELP_SDK_ROOT = (
    "Pico SDK root (default: infer from linker script path(s) or the current directory)"
)
CLI_HELP_PLATFORM = (
    "Chip name under src/; src/<CHIP>/pico_platform must exist. "
    "Default: infer from each memmap path."
)
CLI_MSG_SDK_ROOT_INFER_FAILED = (
    "error: could not infer SDK root; pass --sdk-root or run from the SDK tree"
)

# Strip ANSI sequences when measuring visible width for column alignment.
_ANSI_ESCAPE_RE = re.compile(r"\x1b\[[0-9;]*m")


def _display_width(s: str) -> int:
    return len(_ANSI_ESCAPE_RE.sub("", s))

# SDK hook fragments (default_text_excludes.incl, default_rodata_excludes.incl, etc)
_DEFAULT_EXCLUDES_NAME_RE = re.compile(r"^default_.*_excludes\.incl$")
_DEFAULT_EXCLUDES_INDENT = " " * 8


def _indent_default_excludes_body(text: str) -> str:
    """Prefix each non-empty line with 8 spaces when inlining excludes hooks."""
    out: list[str] = []
    for line in text.splitlines(keepends=True):
        if line.strip() == "":
            out.append(line)
        else:
            out.append(_DEFAULT_EXCLUDES_INDENT + line)
    return "".join(out)


def _strip_comments(text: str) -> str:
    text = _BLOCK_COMMENT_RE.sub("", text)
    lines = []
    for line in text.splitlines():
        if "/*" in line:
            line = line[: line.index("/*")]
        if "//" in line:
            line = line[: line.index("//")]
        lines.append(line)
    return "\n".join(lines)


def find_includes(text: str) -> list[str]:
    out: list[str] = []
    cleaned = _strip_comments(text)
    for m in _INCLUDE_RE.finditer(cleaned):
        name = m.group("quoted") or m.group("bare")
        if name:
            out.append(name)
    return out


def classify(path: Path) -> str:
    """Short SDK path label for a resolved linker fragment (for the tree column)."""
    s = path.resolve().as_posix()
    if "/rp2_common/pico_standard_link/" in s:
        return "rp2_common/pico_standard_link"
    if "/pico_platform/" in s:
        for chip in ("rp2040", "rp2350"):
            if f"/{chip}/pico_platform/" in s:
                return f"{chip}/pico_platform"
        return "pico_platform"
    return "other"


def infer_sdk_root(start: Path) -> Path | None:
    """Walk parents for a directory that looks like the Pico SDK root."""
    p = start.resolve()
    for parent in [p, *p.parents]:
        if (parent / "src" / "rp2_common" / "pico_standard_link").is_dir():
            return parent
    return None


def infer_platform_chip(memmap_ld: Path) -> str | None:
    """Derive rp2040 / rp2350 from .../src/<chip>/pico_platform/...."""
    parts = memmap_ld.resolve().parts
    for i, part in enumerate(parts):
        if part == "pico_platform" and i > 0:
            return parts[i - 1]
    return None


def explicit_platform_root(sdk_root: Path, platform: str) -> Path:
    """``sdk_root/src/<platform>/pico_platform`` (resolved)."""
    return (sdk_root / "src" / platform / "pico_platform").resolve()


def validate_explicit_platform(sdk_root: Path, platform: str) -> str | None:
    """
    When ``--platform`` is passed, require ``src/<platform>/pico_platform`` to exist.
    Return an error message, or None if valid.
    """
    root = explicit_platform_root(sdk_root, platform)
    if root.is_dir():
        return None
    return f"no such platform directory: {root}"


def default_search_dirs(
    sdk_root: Path,
    memmap_ld: Path,
    platform: str | None,
) -> list[Path]:
    """
    Inferred SDK -L sequence (after any user -L from the CLI): same order as
    PICO_LINKER_SCRIPT_INCLUDE_DIRS — platform script_include, then
    pico_standard_link/script_include.
    """
    chip = platform or infer_platform_chip(memmap_ld)
    dirs: list[Path] = []
    if chip:
        plat = sdk_root / "src" / chip / "pico_platform" / "script_include"
        if plat.is_dir():
            dirs.append(plat)
    std = sdk_root / "src" / "rp2_common" / "pico_standard_link" / "script_include"
    if std.is_dir():
        dirs.append(std)
    return dirs


def resolve_include(name: str, search_dirs: list[Path]) -> tuple[Path, Path] | None:
    """First path under `search_dirs`; returns `(file, matched_dir)`."""
    for d in search_dirs:
        c = d / name
        try:
            if c.is_file():
                dr = d.resolve()
                return (c.resolve(), dr)
        except OSError:
            continue
    return None


def flatten_linker_script(path: Path, search_dirs: list[Path], stack: tuple[Path, ...]) -> str:
    """
    Return `path`'s text with every resolvable `INCLUDE` replaced by the
    recursively flattened contents of that file. Unresolvable names in
    `IGNORED_INCLUDE_NAMES` keep the original `INCLUDE` line. Circular
    includes raise `ValueError`.
    """
    path = path.resolve()
    if path in stack:
        chain = " -> ".join(str(p) for p in (*stack, path))
        raise ValueError(f"circular INCLUDE: {chain}")
    text = path.read_text(encoding="utf-8", errors="replace")
    new_stack = (*stack, path)

    def repl(m: re.Match) -> str:
        name = m.group("quoted") or m.group("bare")
        if not name:
            return m.group(0)
        if name in IGNORED_INCLUDE_NAMES:
            return m.group(0)
        hit = resolve_include(name, search_dirs)
        if hit is None:
            raise ValueError(f'unresolved INCLUDE {name!r} while expanding {path}')
        resolved = hit[0]
        inner = flatten_linker_script(resolved, search_dirs, new_stack)
        if _DEFAULT_EXCLUDES_NAME_RE.match(resolved.name):
            inner = _indent_default_excludes_body(inner)
        return inner if inner.endswith("\n") or inner == "" else inner + "\n"

    return _INCLUDE_RE.sub(repl, text)


def _strip_line_comments(text: str) -> str:
    out: list[str] = []
    for line in text.splitlines(keepends=True):
        i = line.find("//")
        if i < 0:
            out.append(line)
            continue
        keep_nl = line.endswith("\n")
        out.append(line[:i] + ("\n" if keep_nl else ""))
    return "".join(out)


def _merge_named_blocks(text: str, name: str, token: str) -> str:
    pat = re.compile(rf"\b{name}\b")
    n = len(text)
    i = 0
    out: list[str] = []
    bodies: list[str] = []
    emitted_token = False

    while i < n:
        m = pat.search(text, i)
        if not m:
            out.append(text[i:])
            break
        start = m.start()
        out.append(text[i:start])
        j = m.end()
        while j < n and text[j].isspace():
            j += 1
        if j >= n or text[j] != "{":
            out.append(text[start:m.end()])
            i = m.end()
            continue

        depth = 0
        k = j
        while k < n:
            ch = text[k]
            if ch == "{":
                depth += 1
            elif ch == "}":
                depth -= 1
                if depth == 0:
                    k += 1
                    break
            k += 1

        if depth != 0:
            out.append(text[start:])
            i = n
            break

        bodies.append(text[j + 1 : k - 1].strip("\n"))
        if not emitted_token:
            out.append(token + "\n")
            emitted_token = True
        i = k

    merged = "\n".join(b for b in bodies if b.strip())
    block = "" if not merged else f"{name}\n{{\n{merged}\n}}\n"
    return "".join(out).replace(token, block)


def postprocess_flattened(text: str) -> str:
    """Remove comments and merge repeated top-level MEMORY/SECTIONS blocks."""
    text = _BLOCK_COMMENT_RE.sub("", text)
    text = _strip_line_comments(text)
    text = _merge_named_blocks(text, "MEMORY", "__MERGED_MEMORY_BLOCK__")
    text = _merge_named_blocks(text, "SECTIONS", "__MERGED_SECTIONS_BLOCK__")
    text = re.sub(r"\n{3,}", "\n\n", text)
    return text.strip() + "\n"


def stdout_use_color() -> bool:
    """True if ANSI color is allowed on stdout (TTY and ``NO_COLOR`` unset)."""
    return sys.stdout.isatty() and os.environ.get("NO_COLOR", "") == ""


def fold_docstring_for_argparse(doc: str) -> str:
    """Fold single newlines to spaces; keep paragraph breaks (blank lines)."""
    parts: list[str] = []
    for block in doc.strip().split("\n\n"):
        folded = re.sub(r"[ \t\r\f\v]+", " ", block.replace("\n", " ")).strip()
        if folded:
            parts.append(folded)
    return "\n\n".join(parts)


def collect_unresolved_include_paths(
    nodes: list[ChildRow], acc: list[Path] | None = None
) -> list[Path]:
    """Collect leaf INCLUDE targets under ``nodes`` that do not exist on disk."""
    if acc is None:
        acc = []
    for path, subs, _ in nodes:
        if not path.exists():
            acc.append(path)
        collect_unresolved_include_paths(subs, acc)
    return acc


def format_memmap_annotate_status_line(tag: str, msg: str) -> str:
    """Color ``updated`` green and ``needs updating`` red when :func:`stdout_use_color`."""
    if not stdout_use_color():
        return f"{tag}: {msg}"
    if msg == "updated":
        return f"{tag}: \033[32m{msg}\033[0m"
    if msg == "needs updating":
        return f"{tag}: \033[31m{msg}\033[0m"
    return f"{tag}: {msg}"


def walk_includes(
    root: Path,
    search_dirs: list[Path],
    user_l_set: frozenset[Path],
    user_l_labels: dict[Path, str],
    visited: set[Path] | None = None,
) -> tuple[Path, list[ChildRow]]:
    """
    Returns (file, children) where each child is
    (included_file, its_children, cli_-L_display_or_none).
    """
    if visited is None:
        visited = set()
    root = root.resolve()
    if root in visited:
        return (root, [])
    visited.add(root)

    try:
        text = root.read_text(encoding="utf-8", errors="replace")
    except OSError as e:
        print(f"warning: could not read {root}: {e}", file=sys.stderr)
        return (root, [])

    children: list[ChildRow] = []
    for inc_name in find_includes(text):
        if inc_name in IGNORED_INCLUDE_NAMES:
            continue
        hit = resolve_include(inc_name, search_dirs)
        if hit is None:
            placeholder = Path(inc_name)
            children.append((placeholder, [], None))
            continue
        resolved, matched_dir = hit
        user_l_str = (
            user_l_labels[matched_dir]
            if matched_dir in user_l_set
            else None
        )
        sub_root, sub_children = walk_includes(
            resolved, search_dirs, user_l_set, user_l_labels, visited
        )
        children.append((sub_root, sub_children, user_l_str))

    return (root, children)


def iter_tree_rows(
    nodes: list[ChildRow],
    prefix: str = "",
    *,
    color: bool,
) -> Iterable[tuple[str, str]]:
    """Yield `(left_column, label_column)` for each tree line (label is two spaces + origin)."""
    green = "\033[32m" if color else ""
    reset = "\033[0m" if color else ""
    for i, (path, subs, user_l_str) in enumerate(nodes):
        is_last = i == len(nodes) - 1
        branch = "└── " if is_last else "├── "
        exists = path.exists()
        miss = "" if exists else "  (unresolved)"
        if user_l_str is not None:
            label = f"  {user_l_str}"
        else:
            label = f"  {classify(path)}"
        name_part = (
            f"{green}{path.name}{reset}" if user_l_str else path.name
        )
        left = f"{prefix}{branch}{name_part}{miss}"
        yield (left, label)
        if subs:
            ext = "    " if is_last else "│   "
            yield from iter_tree_rows(subs, prefix + ext, color=color)


def format_tree_lines(
    root_node: Path, root_children: list[ChildRow], *, color: bool
) -> list[str]:
    """Same layout as printed by ``print_tree_two_columns``, as lines without trailing newlines."""
    root_label = f"  {classify(root_node)}"
    rows: list[tuple[str, str]] = [(root_node.name, root_label)]
    rows.extend(iter_tree_rows(root_children, "", color=color))
    max_w = max(_display_width(left) for left, _ in rows)
    out: list[str] = []
    for left, label in rows:
        pad = max_w - _display_width(left)
        out.append(left + " " * pad + label)
    return out


def print_tree_two_columns(root_node: Path, root_children: list[ChildRow], *, color: bool) -> None:
    """Print root + tree with filenames in the first column and labels aligned in the second."""
    for line in format_tree_lines(root_node, root_children, color=color):
        print(line)


def parse_args(argv: list[str]) -> argparse.Namespace:
    p = argparse.ArgumentParser(
        description=fold_docstring_for_argparse(__doc__),
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    p.add_argument(
        "memmap_ld",
        type=Path,
        help="Base linker script, e.g. src/rp2040/pico_platform/memmap_default.ld",
    )
    p.add_argument(
        "--sdk-root",
        type=Path,
        default=None,
        help=CLI_HELP_SDK_ROOT,
    )
    p.add_argument(
        "--platform",
        default=None,
        metavar="CHIP",
        help=CLI_HELP_PLATFORM,
    )
    p.add_argument(
        "-L",
        "--search-dir",
        dest="search_dirs",
        type=Path,
        action="append",
        default=[],
        metavar="DIR",
        help="Extra override directories to include first",
    )
    p.add_argument(
        "-o",
        "--output",
        dest="output",
        type=Path,
        default=None,
        metavar="FILE",
        help="Also write fully expanded linker script to FILE",
    )
    return p.parse_args(argv)


def main(argv: list[str]) -> int:
    args = parse_args(argv)
    memmap = args.memmap_ld
    if not memmap.is_file():
        print(f"error: not a file: {memmap}", file=sys.stderr)
        return 1

    sdk_root = args.sdk_root
    if sdk_root is None:
        sdk_root = infer_sdk_root(memmap)
    if sdk_root is None:
        print(CLI_MSG_SDK_ROOT_INFER_FAILED, file=sys.stderr)
        return 1

    sdk_root = sdk_root.resolve()
    if args.platform is not None:
        perr = validate_explicit_platform(sdk_root, args.platform)
        if perr:
            print(f"error: {perr}", file=sys.stderr)
            return 1

    user_ldirs: list[Path] = []
    user_l_labels: dict[Path, str] = {}
    for d in args.search_dirs:
        r = d.resolve()
        user_ldirs.append(r)
        user_l_labels[r] = d.as_posix()
    search_dirs = [
        *user_ldirs,
        *default_search_dirs(sdk_root, memmap, args.platform),
    ]

    visited: set[Path] = set()
    user_l_set = frozenset(user_ldirs)
    root_node, root_children = walk_includes(
        memmap.resolve(), search_dirs, user_l_set, user_l_labels, visited
    )

    use_color = stdout_use_color()
    print_tree_two_columns(root_node, root_children, color=use_color)

    missing = collect_unresolved_include_paths(root_children)
    if missing:
        print(
            "\nUnresolved INCLUDE targets (add -L if needed):",
            file=sys.stderr,
        )
        for m in missing:
            print(f"  {m.name}", file=sys.stderr)
        return 2

    if args.output is not None:
        try:
            flat = flatten_linker_script(memmap.resolve(), search_dirs, ())
            flat = postprocess_flattened(flat)
        except OSError as e:
            print(f"error: {e}", file=sys.stderr)
            return 1
        except ValueError as e:
            print(f"error: {e}", file=sys.stderr)
            return 2
        args.output.write_text(flat, encoding="utf-8")

    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
