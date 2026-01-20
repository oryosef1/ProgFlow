#!/bin/bash
# macOS Packaging Script for ProgFlow
# Creates a signed and notarized DMG installer

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_DIR/build/macos-release"
OUTPUT_DIR="$PROJECT_DIR/dist"
APP_NAME="ProgFlow"
VERSION="${VERSION:-1.0.0}"

echo "=== ProgFlow macOS Packaging ==="
echo "Version: $VERSION"

# Build if needed
if [ ! -d "$BUILD_DIR" ]; then
    echo "Building release..."
    cmake --preset macos-release
    cmake --build --preset macos-release
fi

# Create output directory
mkdir -p "$OUTPUT_DIR"

# App bundle location
APP_BUNDLE="$BUILD_DIR/ProgFlow_artefacts/Release/$APP_NAME.app"
PLUGIN_VST3="$BUILD_DIR/ProgFlowPlugin_artefacts/Release/VST3/ProgFlow.vst3"
PLUGIN_AU="$BUILD_DIR/ProgFlowPlugin_artefacts/Release/AU/ProgFlow.component"

if [ ! -d "$APP_BUNDLE" ]; then
    echo "Error: App bundle not found at $APP_BUNDLE"
    exit 1
fi

echo "Creating DMG..."

# Create temporary directory for DMG contents
DMG_TEMP="$OUTPUT_DIR/dmg-temp"
rm -rf "$DMG_TEMP"
mkdir -p "$DMG_TEMP"

# Copy app
cp -R "$APP_BUNDLE" "$DMG_TEMP/"

# Create Applications symlink
ln -s /Applications "$DMG_TEMP/Applications"

# Copy plugins if they exist
if [ -d "$PLUGIN_VST3" ]; then
    mkdir -p "$DMG_TEMP/Plugins/VST3"
    cp -R "$PLUGIN_VST3" "$DMG_TEMP/Plugins/VST3/"
fi

if [ -d "$PLUGIN_AU" ]; then
    mkdir -p "$DMG_TEMP/Plugins/AU"
    cp -R "$PLUGIN_AU" "$DMG_TEMP/Plugins/AU/"
fi

# Create DMG
DMG_NAME="ProgFlow-$VERSION-macOS.dmg"
DMG_PATH="$OUTPUT_DIR/$DMG_NAME"
rm -f "$DMG_PATH"

hdiutil create -volname "ProgFlow $VERSION" \
    -srcfolder "$DMG_TEMP" \
    -ov -format UDZO \
    "$DMG_PATH"

# Clean up
rm -rf "$DMG_TEMP"

echo ""
echo "=== Package Created ==="
echo "DMG: $DMG_PATH"
echo ""

# Code signing (if identity provided)
if [ -n "$CODESIGN_IDENTITY" ]; then
    echo "Signing DMG..."
    codesign --sign "$CODESIGN_IDENTITY" "$DMG_PATH"
    echo "DMG signed."
fi

# Notarization (if credentials provided)
if [ -n "$APPLE_ID" ] && [ -n "$APPLE_TEAM_ID" ]; then
    echo "Submitting for notarization..."
    xcrun notarytool submit "$DMG_PATH" \
        --apple-id "$APPLE_ID" \
        --team-id "$APPLE_TEAM_ID" \
        --password "$APPLE_PASSWORD" \
        --wait

    echo "Stapling notarization ticket..."
    xcrun stapler staple "$DMG_PATH"
    echo "Notarization complete."
fi

echo ""
echo "=== Done ==="
ls -lh "$DMG_PATH"
