#!/bin/sh
VERSION="$1"
sudo xcode-select -s /Applications/Xcode.app/

SIGNER="Developer ID Application: Meltytech, LLC (Y6RX44QG2G)"
find ~/Desktop/Shotcut.app -type d -name __pycache__ -exec rm -r {} \+
find ~/Desktop/Shotcut.app/Contents \( -name '*.o' -or -name '*.a' -or -name '*.dSYM' \) -exec rm -rf {} \;
xattr -cr ~/Desktop/Shotcut.app

# Strip any pre-existing signatures so we can overwrite (Qt SDK, etc.)
find ~/Desktop/Shotcut.app/Contents -type f \( -name '*.dylib' -o -name '*.so' \) -exec \
  codesign --remove-signature {} \; 2>/dev/null || true
find ~/Desktop/Shotcut.app/Contents -type d -name '*.framework' -exec \
  codesign --remove-signature {} \; 2>/dev/null || true
find ~/Desktop/Shotcut.app/Contents/MacOS -type f -exec \
  codesign --remove-signature {} \; 2>/dev/null || true

# Re-sign all dylibs and plugins
find ~/Desktop/Shotcut.app/Contents -type f \( -name '*.dylib' -o -name '*.so' \) -exec \
  codesign --options=runtime --timestamp --force --verbose --sign "$SIGNER" \
    --preserve-metadata=identifier,entitlements \
    {} \;

# Re-sign executables with entitlements
find ~/Desktop/Shotcut.app/Contents/MacOS -type f -exec \
  codesign --options=runtime --timestamp --force --verbose --sign "$SIGNER" \
    --preserve-metadata=identifier,entitlements \
    --entitlements ./notarization.entitlements \
    {} \;

# Re-sign the app bundle last
codesign --options=runtime --timestamp --force --verbose --sign "$SIGNER" \
  --preserve-metadata=identifier,entitlements \
  --entitlements ./notarization.entitlements --generate-entitlement-der \
  ~/Desktop/Shotcut.app

codesign --verify --deep --strict --verbose=4 ~/Desktop/Shotcut.app
spctl -a -t exec -vv ~/Desktop/Shotcut.app

# Create DMG with custom background and layout
TMP=$(mktemp -d)
DMGDIR="$TMP/dmg"
mkdir -p "$DMGDIR/.background"

# Move app and create Applications symlink
mv ~/Desktop/Shotcut.app "$DMGDIR/"
ln -s /Applications "$DMGDIR/Applications"

# Copy background image
cp ../packaging/macos/dmg-background.png "$DMGDIR/.background/"

# Create initial DMG (writable)
rm -f ~/Desktop/shotcut-macos-${VERSION}.dmg
rm -f ~/Desktop/shotcut-macos-${VERSION}-temp.dmg
hdiutil create -srcfolder "$DMGDIR" -volname Shotcut -format UDRW -size 1500m -fs HFS+ \
  ~/Desktop/shotcut-macos-${VERSION}-temp.dmg

# Mount the temporary DMG
device=$(hdiutil attach -readwrite -noverify ~/Desktop/shotcut-macos-${VERSION}-temp.dmg | \
         egrep '^/dev/' | sed 1q | awk '{print $1}')

# Wait for mount
sleep 2

# Run AppleScript to set up the DMG window
osascript > /dev/null <<EOF
tell application "Finder"
    tell disk "Shotcut"
        open
        set current view of container window to icon view
        set toolbar visible of container window to false
        set statusbar visible of container window to false
        set the bounds of container window to {400, 100, 1000, 500}
        set viewOptions to the icon view options of container window
        set arrangement of viewOptions to not arranged
        set icon size of viewOptions to 72
        set background picture of viewOptions to file ".background:dmg-background.png"
        set position of item "Shotcut.app" of container window to {150, 180}
        set position of item "Applications" of container window to {450, 180}
        close
        open
        update without registering applications
        delay 2
    end tell
end tell
EOF

# Unmount the DMG
hdiutil detach "${device}"
sleep 2

# Convert to compressed read-only DMG
hdiutil convert ~/Desktop/shotcut-macos-${VERSION}-temp.dmg \
  -format UDBZ -o ~/Desktop/shotcut-macos-${VERSION}.dmg

# Clean up
rm -f ~/Desktop/shotcut-macos-${VERSION}-temp.dmg
rm -rf "$TMP"

./notarize.sh ~/Desktop/shotcut-macos-${VERSION}.dmg
./staple.sh ~/Desktop/shotcut-macos-${VERSION}.dmg

echo Now run:
echo sudo xcode-select -s /Library/Developer/CommandLineTools
