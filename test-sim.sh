#!/usr/bin/env bash
# Builds the iOS simulator target, then installs and launches it in a
# booted Simulator so you can see it run without needing a device,
# provisioning profile, or signing team (Simulator apps don't need real
# code signing).
#
# Usage: ./test-sim.sh [--no-build] ["iPhone 16"]
#   --no-build       Skip configure+build, just reinstall/relaunch whatever
#                     was built last (e.g. after only editing a WAD/asset,
#                     or to relaunch after it crashed).
#   Optional arg:     simulator device name (default: whatever's already
#                     booted, or "iPhone 16" if nothing is booted).

set -euo pipefail

export DEVELOPER_DIR="/Applications/Xcode.app/Contents/Developer"

XCODE_TOOLCHAIN="$DEVELOPER_DIR/Toolchains/XcodeDefault.xctoolchain/usr/bin"

# Put Xcode's compiler/tools first, but KEEP Homebrew and the normal
# shell PATH so cmake and other tools remain available.
export PATH="$XCODE_TOOLCHAIN:$DEVELOPER_DIR/usr/bin:/opt/homebrew/bin:/opt/homebrew/sbin:$PATH"

export CC="$XCODE_TOOLCHAIN/clang"
export CXX="$XCODE_TOOLCHAIN/clang++"
export AR="$XCODE_TOOLCHAIN/ar"
export RANLIB="$XCODE_TOOLCHAIN/ranlib"
export STRIP="$XCODE_TOOLCHAIN/strip"
export NM="$XCODE_TOOLCHAIN/nm"
export INSTALL_NAME_TOOL="$XCODE_TOOLCHAIN/install_name_tool"
# NOTE: deliberately not exporting LD. With the Xcode generator + this
# ios.toolchain.cmake, an LD env var makes CMake's C-compiler-identification
# step resolve CMAKE_C_COMPILER to $LD instead of clang (CXX is unaffected),
# so "target_compile_features" fails with "no known features for C
# compiler". Xcode's own build system handles linking regardless.

echo "==> developer dir: $DEVELOPER_DIR"
echo "==> cmake: $(command -v cmake)"
echo "==> clang: $(xcrun --sdk iphonesimulator --find clang)"
echo "==> SDK: $(xcrun --sdk iphonesimulator --show-sdk-path)"
echo "==> install_name_tool: $(command -v install_name_tool)"

HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$HERE/build-ios/simulator"
DEPLOYMENT_TARGET=15.0
BUNDLE_ID="org.stjr.srb2"
SKIP_BUILD=0
DEVICE_NAME=""

while [ $# -gt 0 ]; do
	case "$1" in
		--no-build|--skip-build)
			SKIP_BUILD=1
			shift
			;;
		*)
			DEVICE_NAME="$1"
			shift
			;;
	esac
done
DEVICE_NAME="${DEVICE_NAME:-iPhone 18 Pro Max}"

if [ "$SKIP_BUILD" -eq 1 ]; then
	echo "==> skipping configure+build (--no-build)"
else
	# Every `find ... -iname "*.app" | head -1` below assumes there's
	# exactly one .app under bin/Release -- the one this build produces. A
	# stale one left over from a previous build under a different name (an
	# older git branch, or a since-fixed bug in the CMake asset-install
	# destination) can silently win that race instead -- this has actually
	# happened. Clearing it up front removes the ambiguity entirely.
	rm -rf "$BUILD_DIR/bin/Release"

	echo "==> configuring"
	cmake -S "$HERE" -B "$BUILD_DIR" -G Xcode \
		-DCMAKE_TOOLCHAIN_FILE="$HERE/cmake/Modules/ios.toolchain.cmake" \
		-DPLATFORM=SIMULATORARM64 \
		-DDEPLOYMENT_TARGET="$DEPLOYMENT_TARGET" \
		-DSRB2_CONFIG_EXTERNAL_ASSETS=ON \
		-DSRB2_CONFIG_ASSET_DIRECTORY="$HERE/assets/installer" \
		-DCMAKE_BUILD_TYPE=Debug

	echo "==> building"
	cmake --build "$BUILD_DIR" --config Release -- -quiet

	# assets/CMakeLists.txt's asset-copying (SRB2_CONFIG_EXTERNAL_ASSETS) is
	# wired through install(DIRECTORY ...), which only runs on `cmake
	# --install`, not `cmake --build` -- so without this, the .app below
	# never gets srb2.srb/patch.dta/etc. copied in. Installing with --prefix
	# pointed back at the build product directory augments the just-built
	# bundle in place rather than producing a separate copy elsewhere.
	echo "==> installing assets into bundle"
	cmake --install "$BUILD_DIR" --config Release --prefix "$BUILD_DIR/bin/Release"

	# This target has no native Xcode Resources build phase (everything
	# else in the bundle is copied in by our own cmake --install step
	# above, not Xcode's build system), so src/sdl/srb2.icon (an Icon
	# Composer bundle -- supports light/dark/tinted/clear appearances)
	# never gets picked up by actool automatically the way it would in a
	# normal Xcode project. Compile it ourselves and drop Assets.car
	# straight into the (flat, iOS-style) bundle root.
	echo "==> compiling app icon"
	APP_PATH_FOR_ICON="$(find "$BUILD_DIR/bin/Release" -maxdepth 1 -iname "*.app" | head -1)"
	PARTIAL_PLIST="$(mktemp)"
	xcrun actool --compile "$APP_PATH_FOR_ICON" \
		--platform iphonesimulator \
		--minimum-deployment-target "$DEPLOYMENT_TARGET" \
		--app-icon srb2 \
		--output-partial-info-plist "$PARTIAL_PLIST" \
		"$HERE/src/sdl/srb2.icon" \
		> /dev/null
	# actool's own compile step, run inside a real Xcode project, would
	# have Xcode merge this partial plist into Info.plist automatically.
	# CFBundleIconName alone (already in Info.plist.in) isn't enough --
	# SpringBoard's actual icon lookup here needs the CFBundleIcons/
	# CFBundleIcons~ipad dictionaries this generates too (which point at
	# the legacy loose "AppIcon60x60"-style files actool also drops
	# alongside Assets.car), so merge it in ourselves.
	/usr/libexec/PlistBuddy -c "Merge $PARTIAL_PLIST :" "$APP_PATH_FOR_ICON/Info.plist"
	rm -f "$PARTIAL_PLIST"
fi

APP_PATH="$(find "$BUILD_DIR/bin/Release" -maxdepth 1 -iname "*.app" 2>/dev/null | head -1)"
if [ -z "$APP_PATH" ]; then
	if [ "$SKIP_BUILD" -eq 1 ]; then
		echo "error: no .app found under $BUILD_DIR/bin/Release — can't use --no-build before building at least once" >&2
	else
		echo "error: no .app found under $BUILD_DIR/bin/Release" >&2
	fi
	exit 1
fi
echo "==> using: $APP_PATH"

# Boot a simulator if none is already running.
BOOTED_UDID="$(xcrun simctl list devices booted -j | /usr/bin/python3 -c \
	'import json,sys; d=json.load(sys.stdin)["devices"]; print(next((dev["udid"] for devs in d.values() for dev in devs), ""))')"
if [ -z "$BOOTED_UDID" ]; then
	echo "==> no simulator booted, booting \"$DEVICE_NAME\""
	xcrun simctl boot "$DEVICE_NAME"
	# Best-effort: open the simulator GUI so you can see it. Not required —
	# install/launch below work headlessly either way — so don't fail the
	# script if the GUI app isn't where expected. Newer Xcode versions
	# consolidated the standalone Simulator.app into DeviceHub.app.
	if [ -d "/Applications/Xcode.app/Contents/Applications/DeviceHub.app" ]; then
		open "/Applications/Xcode.app/Contents/Applications/DeviceHub.app" 2>/dev/null || true
	else
		open -a Simulator 2>/dev/null || true
	fi
	# Give the simulator a moment to finish booting before installing.
	xcrun simctl bootstatus "$DEVICE_NAME" -b
fi

echo "==> installing"
xcrun simctl install booted "$APP_PATH"

echo "==> launching (streaming console output, Ctrl-C to stop watching)"
xcrun simctl launch --console booted "$BUNDLE_ID"
