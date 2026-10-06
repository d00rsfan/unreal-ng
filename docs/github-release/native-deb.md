# Native Ubuntu DEB

`UnrealNG-Suite-Ubuntu-26.04-x86_64.deb` targets Ubuntu 26.04 LTS on x86_64.
It uses the distribution's shared libraries and Qt plugins, without a private Qt
directory, bundled shared objects or a static C++ runtime. Install it with apt:

```bash
sudo apt install ./UnrealNG-Suite-Ubuntu-26.04-x86_64.deb
```

The package contains the emulator, screen viewer, video wall, MCP bridge and their
resources, including the MIDI sound bank and unpacked configuration disk images.
It uses the same package name and installation paths as the existing
bundled DEB, so installing one replaces the other. Ubuntu 24.04 and Debian 13 users
should use the existing bundled DEB; this native build is tested on Ubuntu 26.04 only.

## Dependencies

`UNREAL_USE_SYSTEM_LIBS=ON` selects distribution Qt (6.7 or newer), OpenSSL, zlib,
zstd (1.5.7 or newer), XZ liblzma (5.4 or newer), Lua 5.4, JsonCpp and libuuid.
The compiler's C++ runtime is dynamically linked as well. Missing development
packages fail configuration. Use a fresh build directory when switching dependency
modes. This mode cannot be combined with `UNREAL_BUNDLE_QT=ON`.

CPack invokes `dpkg-shlibdeps` to derive runtime package versions from the linked
ELF binaries. Qt's X11/offscreen, Wayland and SVG plugins, plus miniaudio's ALSA
and PulseAudio backends, are declared explicitly because they are loaded dynamically.
Unused Qt build modules do not become runtime dependencies. Apt owns these libraries
and updates them independently of Unreal-NG.

The native release enables OpenSSL for both host TLS used by emulated devices and
Trantor. This does not change the local WebAPI's default plain HTTP listener.

Project-specific code remains compiled in: the patched Drogon/Trantor stack,
the patched GIF encoder, miniz's codec/archive API, header-only helpers and emulator
device/codec implementations. This option replaces the standard packaged dependencies;
it does not claim that every third-party source has been removed. The vendored 7-Zip
LZMA SDK is used only by tests in this mode, as a compatibility oracle for CHD streams.
Production CHD compression uses the shared XZ library's raw LZMA1 API without an end marker.

## Build and check locally

The same Dockerfile and scripts run in the release workflow. From the repository root:

```bash
docker build -f docker/linux/Dockerfile.system-deb -t unreal-ng-system-deb:26.04 docker/linux
mkdir -p "$HOME/.cache/unreal-ng/slots"
docker run --rm --cpus 4 --user "$(id -u):$(id -g)" \
  -v "$PWD:/src" -w /src \
  -v "$HOME/.cache/unreal-ng/slots:/build-slots" \
  -e UNREAL_SLOTS_DIR=/build-slots -e UNREAL_JOBS=4 \
  -e XDG_CACHE_HOME=/src/scratch/system-deb-cache -e XDG_CONFIG_HOME=/src/scratch/system-deb-config \
  -e GIT_CONFIG_COUNT=1 -e GIT_CONFIG_KEY_0=safe.directory -e GIT_CONFIG_VALUE_0=/src \
  unreal-ng-system-deb:26.04 tools/build/system-deb.sh
```

Set the CPU/job counts to at most half of your logical cores (minimum one).
The script uses the shared build/test gates, builds all applications, explicitly
builds and runs `core-tests`, then creates the DEB under `scratch/system-deb-build/packages`.
Override `BUILD_DIR` to change the build directory. `UNREAL_PACKAGE_VERSION` optionally
sets the timestamp version (`YYYYMMDD.HHMMSS`); otherwise CMake generates it.

Check installation in a separate container that has no build dependencies:

```bash
docker run --rm -v "$PWD:/src:ro" -w /src ubuntu:26.04 bash -c '
  set -e
  apt-get update
  apt-get install -y --no-install-recommends binutils /src/scratch/system-deb-build/packages/*.deb
  bash tools/verify-system-deb.sh scratch/system-deb-build/packages/UnrealNG-Suite-Ubuntu-26.04-x86_64.deb
'
```

The verifier checks package contents, launchers, resources, required shared libraries,
missing dependencies, runtime search paths, Qt plugins and offscreen startup.
The release publishing job waits for this verification as well as the existing platform checks.
