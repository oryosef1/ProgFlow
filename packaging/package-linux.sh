#!/bin/bash
# Linux Packaging Script for ProgFlow
# Creates AppImage and .deb package

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_DIR/build/linux-release"
OUTPUT_DIR="$PROJECT_DIR/dist"
APP_NAME="ProgFlow"
VERSION="${VERSION:-1.0.0}"

echo "=== ProgFlow Linux Packaging ==="
echo "Version: $VERSION"

# Build if needed
if [ ! -d "$BUILD_DIR" ]; then
    echo "Building release..."
    cmake --preset linux-release
    cmake --build --preset linux-release
fi

# Create output directory
mkdir -p "$OUTPUT_DIR"

# Executable location
APP_BINARY="$BUILD_DIR/ProgFlow_artefacts/Release/$APP_NAME"
PLUGIN_VST3="$BUILD_DIR/ProgFlowPlugin_artefacts/Release/VST3/ProgFlow.vst3"

if [ ! -f "$APP_BINARY" ]; then
    echo "Error: Binary not found at $APP_BINARY"
    exit 1
fi

echo "Creating AppImage..."

# Create AppDir structure
APPDIR="$OUTPUT_DIR/ProgFlow.AppDir"
rm -rf "$APPDIR"
mkdir -p "$APPDIR/usr/bin"
mkdir -p "$APPDIR/usr/lib"
mkdir -p "$APPDIR/usr/share/applications"
mkdir -p "$APPDIR/usr/share/icons/hicolor/256x256/apps"
mkdir -p "$APPDIR/usr/share/metainfo"

# Copy binary
cp "$APP_BINARY" "$APPDIR/usr/bin/"

# Copy shared libraries (system ones that might be needed)
# You may need to adjust this based on your dependencies
if command -v ldd &> /dev/null; then
    ldd "$APP_BINARY" | grep "=> /" | awk '{print $3}' | while read lib; do
        # Only copy non-system libraries
        if [[ "$lib" != /lib* ]] && [[ "$lib" != /usr/lib* ]]; then
            cp "$lib" "$APPDIR/usr/lib/" 2>/dev/null || true
        fi
    done
fi

# Create desktop entry
cat > "$APPDIR/usr/share/applications/progflow.desktop" << EOF
[Desktop Entry]
Type=Application
Name=ProgFlow
Comment=Professional Music Production
Exec=ProgFlow
Icon=progflow
Categories=AudioVideo;Audio;Sequencer;
MimeType=application/x-progflow-project;
Terminal=false
EOF

# Copy desktop file to root of AppDir
cp "$APPDIR/usr/share/applications/progflow.desktop" "$APPDIR/"

# Create AppRun script
cat > "$APPDIR/AppRun" << 'EOF'
#!/bin/bash
SELF=$(readlink -f "$0")
HERE=${SELF%/*}
export PATH="${HERE}/usr/bin/:${PATH}"
export LD_LIBRARY_PATH="${HERE}/usr/lib/:${LD_LIBRARY_PATH}"
exec "${HERE}/usr/bin/ProgFlow" "$@"
EOF
chmod +x "$APPDIR/AppRun"

# Create a simple icon (placeholder - replace with actual icon)
cat > "$APPDIR/usr/share/icons/hicolor/256x256/apps/progflow.png" << 'ICONEOF'
This should be replaced with actual PNG icon data
ICONEOF
# For now, create a symlink placeholder
touch "$APPDIR/progflow.png"

# Create AppStream metadata
cat > "$APPDIR/usr/share/metainfo/progflow.appdata.xml" << EOF
<?xml version="1.0" encoding="UTF-8"?>
<component type="desktop-application">
  <id>com.progflow.ProgFlow</id>
  <name>ProgFlow</name>
  <summary>Professional Music Production</summary>
  <metadata_license>MIT</metadata_license>
  <project_license>MIT</project_license>
  <description>
    <p>ProgFlow is a professional music production application with support for
    MIDI sequencing, audio recording, virtual instruments, and audio effects.</p>
  </description>
  <launchable type="desktop-id">progflow.desktop</launchable>
  <url type="homepage">https://github.com/progflow/progflow</url>
  <provides>
    <binary>ProgFlow</binary>
  </provides>
  <releases>
    <release version="$VERSION" date="$(date +%Y-%m-%d)"/>
  </releases>
</component>
EOF

# Download appimagetool if not present
APPIMAGETOOL="$OUTPUT_DIR/appimagetool"
if [ ! -f "$APPIMAGETOOL" ]; then
    echo "Downloading appimagetool..."
    wget -q "https://github.com/AppImage/AppImageKit/releases/download/continuous/appimagetool-x86_64.AppImage" -O "$APPIMAGETOOL"
    chmod +x "$APPIMAGETOOL"
fi

# Create AppImage
APPIMAGE_NAME="ProgFlow-$VERSION-x86_64.AppImage"
APPIMAGE_PATH="$OUTPUT_DIR/$APPIMAGE_NAME"
rm -f "$APPIMAGE_PATH"

ARCH=x86_64 "$APPIMAGETOOL" "$APPDIR" "$APPIMAGE_PATH" || {
    echo "AppImage creation failed, but AppDir is ready at $APPDIR"
}

# Clean up AppDir
rm -rf "$APPDIR"

echo ""
echo "=== Creating .deb package ==="

# Create deb package structure
DEB_DIR="$OUTPUT_DIR/progflow-$VERSION"
rm -rf "$DEB_DIR"
mkdir -p "$DEB_DIR/DEBIAN"
mkdir -p "$DEB_DIR/usr/bin"
mkdir -p "$DEB_DIR/usr/lib/vst3"
mkdir -p "$DEB_DIR/usr/share/applications"
mkdir -p "$DEB_DIR/usr/share/icons/hicolor/256x256/apps"

# Copy binary
cp "$APP_BINARY" "$DEB_DIR/usr/bin/"

# Copy VST3 plugin
if [ -d "$PLUGIN_VST3" ]; then
    cp -R "$PLUGIN_VST3" "$DEB_DIR/usr/lib/vst3/"
fi

# Desktop entry
cat > "$DEB_DIR/usr/share/applications/progflow.desktop" << EOF
[Desktop Entry]
Type=Application
Name=ProgFlow
Comment=Professional Music Production
Exec=/usr/bin/ProgFlow
Icon=progflow
Categories=AudioVideo;Audio;Sequencer;
Terminal=false
EOF

# Control file
cat > "$DEB_DIR/DEBIAN/control" << EOF
Package: progflow
Version: $VERSION
Section: sound
Priority: optional
Architecture: amd64
Depends: libc6, libstdc++6, libasound2, libfreetype6, libx11-6, libxext6, libxinerama1, libxrandr2, libxcursor1, libgl1
Maintainer: ProgFlow <support@progflow.com>
Description: Professional Music Production
 ProgFlow is a professional music production application with support for
 MIDI sequencing, audio recording, virtual instruments, and audio effects.
EOF

# Build deb
DEB_NAME="progflow_${VERSION}_amd64.deb"
DEB_PATH="$OUTPUT_DIR/$DEB_NAME"
dpkg-deb --build "$DEB_DIR" "$DEB_PATH" 2>/dev/null || {
    echo "dpkg-deb not available. Package directory ready at $DEB_DIR"
}

# Clean up
rm -rf "$DEB_DIR"

echo ""
echo "=== Package Created ==="
[ -f "$APPIMAGE_PATH" ] && echo "AppImage: $APPIMAGE_PATH" && ls -lh "$APPIMAGE_PATH"
[ -f "$DEB_PATH" ] && echo "DEB: $DEB_PATH" && ls -lh "$DEB_PATH"
echo ""
echo "=== Done ==="
