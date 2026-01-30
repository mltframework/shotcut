#!/bin/sh
VERSION="$1"
sudo xcode-select -s /Applications/Xcode.app/

SIGNER="Developer ID Application: Meltytech, LLC (Y6RX44QG2G)"
find ~/Desktop/Shotcut.app -type d -name __pycache__ -exec rm -r {} \+
find ~/Desktop/Shotcut.app/Contents \( -name '*.o' -or -name '*.a' -or -name '*.dSYM' \) -exec rm {} \;
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

TMP=$(mktemp -d)
mv ~/Desktop/Shotcut.app $TMP
ln -s /Applications $TMP
cp ../COPYING $TMP
rm ~/Desktop/shotcut-macos-${VERSION}.dmg
hdiutil create -srcfolder $TMP -volname Shotcut -format UDBZ -size 1500m \
  ~/Desktop/shotcut-macos-${VERSION}.dmg
rm -rf $TMP

./notarize.sh ~/Desktop/shotcut-macos-${VERSION}.dmg
./staple.sh ~/Desktop/shotcut-macos-${VERSION}.dmg

echo Now run:
echo sudo xcode-select -s /Library/Developer/CommandLineTools
