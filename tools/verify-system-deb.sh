#!/usr/bin/env bash
# Run after apt installs this DEB in a fresh Ubuntu 26.04 container.
set -Eeuo pipefail
trap 'echo "Native DEB verification failed at line $LINENO: $BASH_COMMAND" >&2' ERR
package=${1:?Usage: verify-system-deb.sh path/to/package.deb}
dpkg-deb --info "$package"
test "$(dpkg-query -W -f='${Version}' unreal-ng)" = "$(dpkg-deb -f "$package" Version)"
contents=$(dpkg-deb --fsys-tarfile "$package" | tar -tf -)
if grep -E '\.(so([.][0-9]+)*|a|h|hpp)$|/(plugins|include)/|/qt.conf$' <<< "$contents"; then
    echo "Native DEB contains bundled libraries, plugins or development files" >&2
    exit 1
fi
test ! -d /usr/lib/unreal-ng/lib
test -f /usr/lib/unreal-ng/configs/pentagon512k/unreal.ini
test -f /usr/lib/unreal-ng/rom/README-ROMS.md
test -f /usr/lib/unreal-ng/midi/generaluser-gs.sf2
test -s /usr/lib/unreal-ng/configs/ts-conf/wc-zifi.img
test ! -e /usr/lib/unreal-ng/configs/ts-conf/wc-zifi.img.7z

for app in unreal-qt unreal-screen-viewer unreal-videowall unreal-mcp-bridge; do
    binary=/usr/lib/unreal-ng/$app
    test -x "$binary"
    test "$(readlink -f "/usr/bin/$app")" = "$binary"
    dependencies=$(env -u LD_LIBRARY_PATH ldd "$binary")
    printf '%s\n' "$dependencies"
    if grep -q 'not found' <<< "$dependencies"; then
        exit 1
    fi
    dynamic=$(readelf -d "$binary")
    grep -F '[libstdc++.so' <<< "$dynamic" >/dev/null
    if grep -E '\((RPATH|RUNPATH)\)' <<< "$dynamic"; then
        echo "Unexpected runtime search path in $app" >&2
        exit 1
    fi
done

# These must be ELF dependencies, not libraries incorporated into the binary.
dynamic=$(readelf -d /usr/lib/unreal-ng/unreal-qt)
for library in Qt6Core Qt6Widgets Qt6Svg ssl crypto z zstd lzma lua5.4 jsoncpp uuid stdc++; do
    grep -F "[lib${library}.so" <<< "$dynamic" >/dev/null
done
for plugin in libqxcb.so libqoffscreen.so libqsvgicon.so libqsvg.so; do
    test -n "$(find /usr/lib -path '*/qt6/plugins/*' -name "$plugin" -print -quit)"
done
test -n "$(find /usr/lib -path '*/qt6/plugins/platforms/libqwayland*.so' -print -quit)"
audioLibraries=$(ldconfig -p)
grep -F 'libasound.so.2' <<< "$audioLibraries" >/dev/null
grep -F 'libpulse.so.0' <<< "$audioLibraries" >/dev/null
env -u LD_LIBRARY_PATH -u QT_PLUGIN_PATH -u QTDIR -u QT_ROOT_DIR \
    QT_QPA_PLATFORM=offscreen unreal-qt --help
unreal-mcp-bridge </dev/null
echo "Native DEB verification passed."
