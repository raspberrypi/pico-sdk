#!/usr/bin/env python3
#
# Copyright (c) 2026 Raspberry Pi Ltd.
#
# SPDX-License-Identifier: BSD-3-Clause
#
#
# Generate a CycloneDX 1.6 (JSON) Software Bill of Materials for a Pico SDK binary.
#
# The linker map file of the binary is used to find out which parts of the SDK
# (and of any other component that has an sbom.json manifest) were actually
# linked in, so the SBOM only lists what ended up in the firmware.
#
# Usage (normally invoked by pico_add_sbom() from CMake):
#
# tools/sbom/pico_sbom.py --map app.elf.map --sdk-path <pico-sdk> --name app -o app.elf.cdx.json
#
# Component manifests (sbom.json) may be an object or a list of objects with keys:
#   name        (required) package name
#   path        directory of the component relative to the manifest (default ".")
#   version     version string; if omitted it is taken from git, or for the SDK
#               itself from pico_sdk_version.cmake
#   license     SPDX license expression, or LicenseRef-* (default NOASSERTION)
#   license_file  file (relative to manifest) holding the text of a LicenseRef-*
#   supplier    e.g. "Organization: Raspberry Pi Ltd"
#   url         home page / repository
#   description, cpe, purl, comment
# Where components are nested the most specific (deepest) path wins.

import argparse
import datetime
import json
import os
import re
import subprocess
import sys
import uuid

TOOL_VERSION = "1.0"
MANIFEST_NAME = "sbom.json"
NOASSERTION = "NOASSERTION"
SKIP_DIRS = {".git", "build", "node_modules", "__pycache__"}


def warn(msg):
    print("pico_sbom: warning: " + msg, file=sys.stderr)


def norm(path):
    return os.path.normpath(path).replace("\\", "/")


def is_under(path, directory):
    return path == directory or path.startswith(directory.rstrip("/") + "/")


def git(cwd, *args):
    try:
        out = subprocess.run(["git", "-C", cwd] + list(args), capture_output=True, text=True, check=True)
        return out.stdout.strip()
    except (OSError, subprocess.CalledProcessError):
        return None


def sdk_version(sdk_path):
    values = {}
    try:
        with open(os.path.join(sdk_path, "pico_sdk_version.cmake")) as f:
            for m in re.finditer(r"set\(\s*PICO_SDK_VERSION_(MAJOR|MINOR|REVISION|PRE_RELEASE_ID)\s+(\S+?)\s*\)", f.read()):
                values[m.group(1)] = m.group(2).strip('"')
    except OSError:
        return None
    if not all(k in values for k in ("MAJOR", "MINOR", "REVISION")):
        return None
    version = "{MAJOR}.{MINOR}.{REVISION}".format(**values)
    if values.get("PRE_RELEASE_ID"):
        version += "-" + values["PRE_RELEASE_ID"]
    return version


def git_version(comp_dir, repo_root):
    """Version (and commit) of a git submodule, which may not be checked out; None for plain directories."""
    rel = os.path.relpath(comp_dir, repo_root)
    tree = git(repo_root, "ls-tree", "HEAD", rel)
    fields = tree.split() if tree else []
    if len(fields) < 3 or fields[0] != "160000":
        return None, None
    if os.path.exists(os.path.join(comp_dir, ".git")):
        described = git(comp_dir, "describe", "--tags", "--always")
        if described:
            return described, git(comp_dir, "rev-parse", "HEAD")
    # submodule not checked out: use the commit recorded in the superproject
    return fields[2][:12], fields[2]


class Component:
    def __init__(self, info, manifest_path, sdk_path):
        self.info = info
        self.name = info["name"]
        manifest_dir = os.path.dirname(manifest_path)
        self.directory = norm(os.path.abspath(os.path.join(manifest_dir, info.get("path", "."))))
        self.manifest_dir = manifest_dir
        self.is_sdk = (self.directory == norm(sdk_path))
        self.version = info.get("version")
        self.commit = None
        if not self.version:
            if self.is_sdk:
                self.version = sdk_version(sdk_path)
            else:
                self.version, self.commit = git_version(self.directory, sdk_path)
        self.license = info.get("license", NOASSERTION)
        self.used = False


def load_manifests(search_roots, sdk_path):
    components = []
    for root in search_roots:
        if os.path.isfile(root):
            candidates = [root]
        else:
            candidates = []
            for dirpath, dirnames, filenames in os.walk(root):
                dirnames[:] = [d for d in dirnames if d not in SKIP_DIRS]
                if MANIFEST_NAME in filenames:
                    candidates.append(os.path.join(dirpath, MANIFEST_NAME))
        for manifest in candidates:
            try:
                with open(manifest) as f:
                    data = json.load(f)
            except (OSError, ValueError) as e:
                warn("cannot read {}: {}".format(manifest, e))
                continue
            for info in (data if isinstance(data, list) else [data]):
                if "name" not in info:
                    warn("{}: entry without a name ignored".format(manifest))
                    continue
                components.append(Component(info, manifest, sdk_path))
    return components


# ---- linker map handling ----------------------------------------------------

ARCHIVE_RE = re.compile(r"([^\s()]+\.a)\(([^)\s]+)\)")
OBJECT_RE = re.compile(r"(?<![^\s])([^\s()]+\.(?:o|obj))(?![^\s])")


def map_inputs(map_text):
    """Return the set of input paths (object files and archives) mentioned in a GNU ld map."""
    paths = set(m.group(1) for m in ARCHIVE_RE.finditer(map_text))
    remainder = ARCHIVE_RE.sub(" ", map_text)
    paths.update(m.group(1) for m in OBJECT_RE.finditer(remainder))
    return paths


def resolve_path(path, project_dir, path_maps):
    """Turn a path as it appears in the map into candidate source-tree paths."""
    path = path.replace("\\", "/")
    if ".dir/" in path:
        # CMake object: CMakeFiles/<target>.dir/<source path>.obj, where a source outside the
        # project's source directory has its absolute path appended without the leading slash,
        # so it could be either of those
        path = path.split(".dir/", 1)[1]
        path = re.sub(r"^([A-Za-z])_/", r"\1:/", path)  # CMake's encoding of drive letters
        if os.path.isabs(path):
            return [norm(path)]
        candidates = [norm("/" + path)]
        if project_dir:
            candidates.append(norm(os.path.join(project_dir, path)))
        return candidates
    path = norm(os.path.abspath(path))  # archives are relative to the build directory (our cwd)
    for build_dir, source_dir in path_maps:
        if is_under(path, build_dir):
            return [norm(os.path.join(source_dir, os.path.relpath(path, build_dir)))]
    return [path]


def find_component(path, components):
    best = None
    for comp in components:
        if is_under(path, comp.directory) and (best is None or len(comp.directory) > len(best.directory)):
            best = comp
    return best


# ---- CycloneDX output -----------------------------------------------------------

def bom_ref(name):
    return "pico-sbom-" + re.sub(r"[^A-Za-z0-9.-]", "-", name)


def supplier_entry(supplier):
    # manifests use the SPDX style "Organization: Name"; CycloneDX just wants the name
    return {"name": re.sub(r"^(Organization|Person):\s*", "", supplier)}


def license_entries(license_expr, text=None):
    if not license_expr or license_expr == NOASSERTION:
        return None
    refs = re.findall(r"LicenseRef-[A-Za-z0-9.-]+", license_expr)
    if refs:
        # not an SPDX id: CycloneDX wants a named license, with its text where we have it
        entry = {"name": refs[0][len("LicenseRef-"):]}
        if text:
            entry["text"] = {"contentType": "text/plain", "content": text}
        return [{"license": entry}]
    if re.fullmatch(r"[A-Za-z0-9.+-]+", license_expr):
        return [{"license": {"id": license_expr}}]
    return [{"expression": license_expr}]


def license_text(comp):
    license_file = comp.info.get("license_file")
    if not license_file:
        return None
    try:
        with open(os.path.join(comp.manifest_dir, license_file), encoding="utf-8", errors="replace") as f:
            return f.read()
    except OSError:
        warn("license file {} for {} not found".format(license_file, comp.name))
        return None


def component_entry(comp):
    info = comp.info
    entry = {"type": "library", "bom-ref": bom_ref(comp.name), "name": comp.name}
    if comp.version:
        entry["version"] = comp.version
    if info.get("supplier"):
        entry["supplier"] = supplier_entry(info["supplier"])
    if info.get("description"):
        entry["description"] = info["description"]
    licenses = license_entries(comp.license, license_text(comp))
    if licenses:
        entry["licenses"] = licenses
    # a CPE is only useful for vulnerability matching with a real release version, not a commit or a
    # "tag-N-gHASH" description of a commit between releases
    if info.get("cpe") and comp.version and re.fullmatch(r"v?\d+(\.\d+)*", comp.version):
        entry["cpe"] = info["cpe"].replace("{}", re.sub(r"^v", "", comp.version or "*"))
    if info.get("purl"):
        # without a version a purl cannot be matched against vulnerability data, so drop the "@"
        entry["purl"] = info["purl"].replace("@{}", "@" + comp.version if comp.version else "")
    if info.get("url"):
        entry["externalReferences"] = [{"type": "website", "url": info["url"]}]
    properties = []
    if comp.commit:
        properties.append({"name": "pico:git-commit", "value": comp.commit})
    if info.get("comment"):
        properties.append({"name": "pico:comment", "value": info["comment"]})
    if properties:
        entry["properties"] = properties
    return entry


def build_document(args, used):
    app_ref = bom_ref("application-" + args.name)
    app = {"type": "firmware", "bom-ref": app_ref, "name": args.name}
    if args.version:
        app["version"] = args.version
    if args.supplier:
        app["supplier"] = supplier_entry(args.supplier)
    licenses = license_entries(args.license)
    if licenses:
        app["licenses"] = licenses
    return {
        "bomFormat": "CycloneDX",
        "specVersion": "1.6",
        "serialNumber": "urn:uuid:" + str(uuid.uuid4()),
        "version": 1,
        "metadata": {
            "timestamp": datetime.datetime.now(datetime.timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ"),
            "tools": {"components": [{"type": "application", "name": "pico-sdk-sbom", "version": TOOL_VERSION}]},
            "component": app,
        },
        "components": [component_entry(c) for c in used],
        "dependencies": [{"ref": app_ref, "dependsOn": [bom_ref(c.name) for c in used]}]
                        + [{"ref": bom_ref(c.name), "dependsOn": []} for c in used],
    }


def main():
    parser = argparse.ArgumentParser(description="Generate a CycloneDX SBOM for a Pico SDK binary from its linker map")
    parser.add_argument("--map", required=True, help="linker map file of the binary")
    parser.add_argument("--sdk-path", required=True, help="path of the Pico SDK")
    parser.add_argument("--name", required=True, help="name of the application")
    parser.add_argument("--version", help="version of the application")
    parser.add_argument("--license", help="SPDX license expression of the application")
    parser.add_argument("--supplier", help='supplier of the application, e.g. "Organization: Example Ltd"')
    parser.add_argument("--project-dir", help="source directory of the application (to resolve relative paths in the map)")
    parser.add_argument("--path-map", nargs=2, action="append", default=[], metavar=("BUILD_DIR", "SOURCE_DIR"),
                        help="binary directory and the source directory it was built from; may be repeated")
    parser.add_argument("--manifest", action="append", default=[],
                        help="extra sbom.json file or directory to search for sbom.json files; may be repeated")
    parser.add_argument("-o", "--output", required=True, help="CycloneDX JSON file to write")
    args = parser.parse_args()

    sdk_path = norm(os.path.realpath(args.sdk_path))
    project_dir = norm(os.path.realpath(args.project_dir)) if args.project_dir else None
    path_maps = [(norm(os.path.realpath(b)), norm(os.path.realpath(s))) for b, s in args.path_map]

    components = load_manifests([sdk_path] + [os.path.realpath(m) for m in args.manifest], sdk_path)
    if not any(c.is_sdk for c in components):
        sys.exit("pico_sbom: error: no {} found at the root of {}".format(MANIFEST_NAME, sdk_path))

    try:
        with open(args.map, errors="replace") as f:
            map_text = f.read()
    except OSError as e:
        sys.exit("pico_sbom: error: cannot read map file: {}".format(e))

    for path in map_inputs(map_text):
        for candidate in resolve_path(path, project_dir, path_maps):
            comp = find_component(candidate, components)
            if comp:
                comp.used = True
                break

    used = sorted((c for c in components if c.used), key=lambda c: c.name.lower())
    with open(args.output, "w") as f:
        json.dump(build_document(args, used), f, indent=2)
        f.write("\n")
    print("pico_sbom: wrote {} ({} components)".format(args.output, len(used)))


if __name__ == "__main__":
    main()
