# Release workflow testing

Run **Release Build** on a branch other than `master` to build and verify every
package without publishing. Pull requests to `master` that change build or
packaging files run the same checks. Download the results from the workflow run's
**Artifacts** section.

Manual dispatch on `master` replaces the public `continuous` prerelease and its tag.
Pushing a `v*` tag publishes a milestone release. Neither path creates a draft;
use a feature branch for verification before publishing.

## Expected artifacts

| Platform | Package |
|----------|---------|
| Linux x86_64, portable | `UnrealNG-Suite-Linux-x86_64.AppImage` |
| Linux x86_64, bundled Qt DEB | `UnrealNG-Suite-Linux-x86_64.deb` |
| Ubuntu 26.04 x86_64, system libraries | `UnrealNG-Suite-Ubuntu-26.04-x86_64.deb` |
| macOS Intel | `UnrealNG-Suite-macOS-x86_64.dmg` |
| macOS Apple Silicon | `UnrealNG-Suite-macOS-arm64.dmg` |
| Windows x86_64, MSVC | `UnrealNG-Suite-Windows-x86_64.zip` |
| Windows x86_64, MinGW | `UnrealNG-Suite-Windows-x86_64-MinGW.zip` |
| Windows ARM64 | `UnrealNG-Suite-Windows-arm64.zip` |

The publishing job also creates `SHA256SUMS.txt`. Symbol archives are produced
for manual workflow runs by the existing portable builds. The native DEB does
not currently produce a separate symbol archive.

## Linux verification

- The bundled DEB installs in clean Ubuntu 24.04 and 26.04 containers. Checks
  confirm that private Qt libraries/plugins are present and system Qt is absent.
- The native DEB builds with distribution libraries in Ubuntu 26.04 and runs the
  full core test suite, including raw LZMA compatibility tests. A separate clean
  Ubuntu 26.04 container installs it using apt without recommended packages.
  `tools/verify-system-deb.sh` checks its contents, shared dependencies, launchers,
  resources, Qt plugins and offscreen startup.
- The AppImage starts with the Qt SDK environment removed, using extraction mode
  so verification does not require FUSE.

Follow [native DEB packaging](native-deb.md) to reproduce the native build and
clean-install check locally. The regular Linux build image and local commands
are documented in [docker/linux/README.md](../../docker/linux/README.md).

## Before publishing

1. Verify that all build and package-install jobs passed on the feature branch.
2. Download and launch the applicable packages on their target systems. Automated
   offscreen startup does not cover interactive graphics, audio or desktop integration.
3. Confirm ROMs, machine configurations and the display utilities are present.
4. After publishing, verify downloads with `sha256sum -c SHA256SUMS.txt`.

The release job depends on every build and both DEB verification jobs; a failed
native DEB build or install blocks publication alongside the other platforms.
