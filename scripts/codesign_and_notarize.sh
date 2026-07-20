#!/bin/sh
VERSION="$1"
sudo xcode-select -s /Applications/Xcode.app/

SIGNER="Developer ID Application: Meltytech, LLC (Y6RX44QG2G)"
find ~/Desktop/Snapflow.app -type d -name __pycache__ -exec rm -r {} \+
find ~/Desktop/Snapflow.app/Contents \( -name '*.o' -or -name '*.a' -or -name '*.dSYM' \) -exec rm -rf {} \;
xattr -cr ~/Desktop/Snapflow.app

# Strip any pre-existing signatures so we can overwrite (Qt SDK, etc.)
find ~/Desktop/Snapflow.app/Contents -type f \( -name '*.dylib' -o -name '*.so' \) -exec \
  codesign --remove-signature {} \; 2>/dev/null || true
find ~/Desktop/Snapflow.app/Contents -type d -name '*.framework' -exec \
  codesign --remove-signature {} \; 2>/dev/null || true
find ~/Desktop/Snapflow.app/Contents/MacOS -type f -exec \
  codesign --remove-signature {} \; 2>/dev/null || true

# Re-sign all dylibs and plugins
find ~/Desktop/Snapflow.app/Contents -type f \( -name '*.dylib' -o -name '*.so' \) -exec \
  codesign --options=runtime --timestamp --force --verbose --sign "$SIGNER" \
    --preserve-metadata=identifier,entitlements \
    {} \;

# Re-sign executables with entitlements
find ~/Desktop/Snapflow.app/Contents/MacOS -type f -exec \
  codesign --options=runtime --timestamp --force --verbose --sign "$SIGNER" \
    --preserve-metadata=identifier,entitlements \
    --entitlements ./notarization.entitlements \
    {} \;

# Re-sign the app bundle last
codesign --options=runtime --timestamp --force --verbose --sign "$SIGNER" \
  --preserve-metadata=identifier,entitlements \
  --entitlements ./notarization.entitlements --generate-entitlement-der \
  ~/Desktop/Snapflow.app

codesign --verify --deep --strict --verbose=4 ~/Desktop/Snapflow.app
spctl -a -t exec -vv ~/Desktop/Snapflow.app

# Create DMG with custom background and layout
TMP=$(mktemp -d)
DMGDIR="$TMP/dmg"
mkdir -p "$DMGDIR/.background"

# Move app and create Applications symlink
mv ~/Desktop/Snapflow.app "$DMGDIR/"
ln -s /Applications "$DMGDIR/Applications"

# Copy background image
cp ../packaging/macos/dmg-background.png "$DMGDIR/.background/"

# Create initial DMG (writable)
rm -f ~/Desktop/snapflow-macos-${VERSION}.dmg
rm -f ~/Desktop/snapflow-macos-${VERSION}-temp.dmg
hdiutil create -srcfolder "$DMGDIR" -volname Snapflow -format UDRW -size 1500m -fs HFS+ \
  ~/Desktop/snapflow-macos-${VERSION}-temp.dmg

# Mount the temporary DMG
device=$(hdiutil attach -readwrite -noverify ~/Desktop/snapflow-macos-${VERSION}-temp.dmg | \
         egrep '^/dev/' | sed 1q | awk '{print $1}')

# Wait for mount
sleep 2

# Run AppleScript to set up the DMG window
osascript > /dev/null <<EOF
tell application "Finder"
    tell disk "Snapflow"
        open
        set current view of container window to icon view
        set toolbar visible of container window to false
        set statusbar visible of container window to false
        set the bounds of container window to {400, 100, 1000, 500}
        set viewOptions to the icon view options of container window
        set arrangement of viewOptions to not arranged
        set icon size of viewOptions to 72
        set background picture of viewOptions to file ".background:dmg-background.png"
        set position of item "Snapflow.app" of container window to {150, 180}
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
hdiutil convert ~/Desktop/snapflow-macos-${VERSION}-temp.dmg \
  -format UDBZ -o ~/Desktop/snapflow-macos-${VERSION}.dmg

# Clean up
rm -f ~/Desktop/snapflow-macos-${VERSION}-temp.dmg
rm -rf "$TMP"

./notarize.sh ~/Desktop/snapflow-macos-${VERSION}.dmg
./staple.sh ~/Desktop/snapflow-macos-${VERSION}.dmg

echo Now run:
echo sudo xcode-select -s /Library/Developer/CommandLineTools
