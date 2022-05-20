#!/bin/sh
VERSION="$1"
sudo xcode-select -s /Applications/Xcode.app/

SIGNER="Developer ID Application: Meltytech, LLC (Y6RX44QG2G)"
find ~/Desktop/Shotcut.app/Contents/Frameworks -type f -exec codesign --options=runtime -v -s "$SIGNER" {} \;
find ~/Desktop/Shotcut.app/Contents/PlugIns -type f -exec codesign --options=runtime -v -s "$SIGNER" {} \;
find ~/Desktop/Shotcut.app/Contents/Resources -type f -exec codesign --options=runtime -v -s "$SIGNER" {} \;
xattr -cr ~/Desktop/Shotcut.app
codesign --options=runtime -v -s "$SIGNER" \
  --entitlements ./notarization.entitlements \
  ~/Desktop/Shotcut.app/Contents/MacOS/{melt,ffmpeg,ffplay,ffprobe,glaxnimate}
codesign --options=runtime -v -s "$SIGNER" \
  --entitlements ./notarization.entitlements \
  ~/Desktop/Shotcut.app
codesign --verify --deep --strict --verbose=2 ~/Desktop/Shotcut.app
spctl -a -t exec -vv ~/Desktop/Shotcut.app

TMP=$(mktemp -d)
mv ~/Desktop/Shotcut.app $TMP
ln -s /Applications $TMP
cp ../COPYING $TMP
rm ~/Desktop/shotcut-macos-${VERSION}.dmg
hdiutil create -srcfolder $TMP -volname Shotcut -format UDBZ -size 400m \
  ~/Desktop/shotcut-macos-${VERSION}.dmg
rm -rf $TMP

./notarize.sh ~/Desktop/shotcut-macos-${VERSION}.dmg

echo wait for the e-mail from Apple and then run:
echo ./staple.sh ~/Desktop/shotcut-macos-${VERSION}.dmg
echo sudo xcode-select -s /Library/Developer/CommandLineTools
