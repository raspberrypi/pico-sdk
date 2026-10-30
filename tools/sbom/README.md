# SBOM generation

`pico_add_sbom(<target>)` (see `src/cmake/pico_sbom.cmake`) generates a CycloneDX 1.6 JSON Software Bill of
Materials, `<target>.elf.cdx.json`, after the target is linked. It lists the SDK and any bundled or third party
components that were actually linked into the binary.

The output can be fed to tools such as Dependency-Track, Grype or Trivy to check the components against
vulnerability databases.

## Enabling

Call it after `pico_add_extra_outputs`. Nothing is generated unless you do.

```cmake
pico_add_extra_outputs(my_app)
pico_add_sbom(my_app
    NAME     my_app                                  # default: the target name
    VERSION  1.2.3                                   # application version
    LICENSE  MIT                                     # SPDX license expression of the application
    SUPPLIER "Organization: Example Ltd"
    OUTPUT   ${CMAKE_BINARY_DIR}/my_app.cdx.json     # default: <target>.elf.cdx.json next to the binary
    MANIFEST ${CMAKE_CURRENT_LIST_DIR}/third_party   # extra sbom.json files or directories, see below
)
```

All arguments except the target are optional. Python 3 is required.

## How it works

1. `pico_add_sbom` enables linker map output and adds a post-build step that runs `pico_sbom.py`.
2. The script reads the linker map and collects every object file and archive member that was linked.
3. It loads the `sbom.json` manifests (see below) and matches each linked file to the component whose
   directory contains it; the deepest matching directory wins. A directory can only be described once: if two
   manifests claim the same directory the first one found is used and a warning is printed. The SDK is searched
   before `MANIFEST`, so the manifests shipped in the SDK take precedence over a manifest added later, for
   example by a submodule or by your project.
4. Every component with at least one linked file is written to the SBOM. Components that are not linked, for
   example btstack in a project that does not use Bluetooth, are left out.

Not covered: toolchain libraries (libc, libgcc), Bazel builds, and libraries that `PICO_*_PATH` points outside
the SDK (give those a manifest, see below).

## Component manifests

A component is a directory described by an `sbom.json` file. The SDK ships manifests for itself
(`sbom.json`), its third party libraries (`lib/sbom.json`) and the code it carries copies of
(`src/rp2_common/pico_printf/sbom.json`, `src/rp2_common/cmsis/stub/sbom.json`).

### Your own third party libraries

Put an `sbom.json` next to the library and tell `pico_add_sbom` where to find it:

```
my_project/
  CMakeLists.txt
  main.c
  third_party/
    mylib/
      sbom.json
      mylib.c
```

```cmake
pico_add_sbom(my_app MANIFEST ${CMAKE_CURRENT_LIST_DIR}/third_party)
```

`MANIFEST` takes files or directories; directories are searched recursively for `sbom.json`.

```json
{
    "name": "mylib",
    "path": ".",
    "version": "2.1.0",
    "license": "MIT",
    "supplier": "Organization: Acme Ltd",
    "url": "https://github.com/acme/mylib",
    "description": "Example library",
    "cpe": "cpe:2.3:a:acme:mylib:{}:*:*:*:*:*:*:*",
    "purl": "pkg:github/acme/mylib@{}"
}
```

A manifest holds one object, or a list of objects for several components (as `lib/sbom.json` does).

| Key | Meaning |
| --- | --- |
| `name` | Required. Component name. |
| `path` | Directory of the component relative to the manifest. Default `.`. |
| `version` | Version. If omitted: the SDK version for the SDK itself, the git tag or commit for a git submodule, otherwise unknown. |
| `license` | SPDX id (`MIT`), SPDX expression (`Apache-2.0 OR GPL-2.0-or-later`) or `LicenseRef-<name>` for a custom license. Default unknown. |
| `license_file` | File, relative to the manifest, with the text of a `LicenseRef-` license. It is embedded in the SBOM. |
| `supplier` | `Organization: Name` or `Person: Name`. |
| `url`, `description`, `comment` | Free text. |
| `cpe`, `purl` | Identifiers used for vulnerability matching. `{}` is replaced by the version. |

Libraries built into a static library (`add_subdirectory` and `target_link_libraries`) as well as sources
compiled straight into the application are both detected.

## Output

`my_app.elf.cdx.json`, abridged:

```json
{
  "bomFormat": "CycloneDX",
  "specVersion": "1.6",
  "serialNumber": "urn:uuid:...",
  "metadata": {
    "timestamp": "2026-10-17T12:00:00Z",
    "tools": { "components": [{ "type": "application", "name": "pico-sdk-sbom", "version": "1.0" }] },
    "component": { "type": "firmware", "bom-ref": "pico-sbom-application-my_app", "name": "my_app", "version": "1.2.3" }
  },
  "components": [
    {
      "type": "library",
      "bom-ref": "pico-sbom-pico-sdk",
      "name": "pico-sdk",
      "version": "2.3.1",
      "supplier": { "name": "Raspberry Pi Ltd" },
      "licenses": [{ "license": { "id": "BSD-3-Clause" } }],
      "purl": "pkg:github/raspberrypi/pico-sdk@2.3.1"
    },
    {
      "type": "library",
      "name": "mbedtls",
      "version": "0bebf8b8c7f0",
      "licenses": [{ "expression": "Apache-2.0 OR GPL-2.0-or-later" }],
      "purl": "pkg:github/Mbed-TLS/mbedtls@0bebf8b8c7f0",
      "properties": [{ "name": "pico:git-commit", "value": "0bebf8b8c7f0..." }]
    }
  ],
  "dependencies": [
    { "ref": "pico-sbom-application-my_app", "dependsOn": ["pico-sbom-pico-sdk", "pico-sbom-mbedtls"] }
  ]
}
```

- The application is the `firmware` component in `metadata`; every linked component is a `library` and the
  application depends on all of them.
- A warning is printed when a `license_file` cannot be read.

## Vulnerability matching

Scanners match on each component's `cpe` and `purl`, so the version has to be a real release:

- A `cpe` is only written when the version is a plain release number, for example a submodule checked out at
  a release tag.
- Submodules that are not checked out, or sit between releases, get a commit based version and a `purl` only.
- To pin a version, set `version` in the manifest.
