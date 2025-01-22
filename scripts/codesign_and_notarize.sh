#!/bin/sh
VERSION="$1"
sudo xcode-select -s /Applications/Xcode.app/

SIGNER="Developer ID Application: Meltytech, LLC (Y6RX44QG2G)"
find ~/Desktop/Shotcut.app -type d -name __pycache__ -exec rm -r {} \+
find ~/Desktop/Shotcut.app/Contents -name '*.o' -exec rm {} \;
find ~/Desktop/Shotcut.app/Contents/lib -type f -exec codesign --options=runtime --force --verbose --sign "$SIGNER" {} \;
find ~/Desktop/Shotcut.app/Contents/Frameworks -type f -exec codesign --options=runtime --force --verbose --sign "$SIGNER" {} \;
find ~/Desktop/Shotcut.app/Contents/PlugIns -type f -exec codesign --options=runtime --force --verbose --sign "$SIGNER" {} \;
find ~/Desktop/Shotcut.app/Contents/Resources -type f -exec codesign --options=runtime --force --verbose --sign "$SIGNER" {} \;
xattr -cr ~/Desktop/Shotcut.app
codesign --options=runtime --force --verbose --sign "$SIGNER" \
  --entitlements ./notarization.entitlements \
  ~/Desktop/Shotcut.app/Contents/MacOS/{melt,ffmpeg,ffplay,ffprobe,glaxnimate,gopro2gpx,whisper-cli}
codesign --options=runtime --force --verbose --sign "$SIGNER" \
  --entitlements ./notarization.entitlements \
  ~/Desktop/Shotcut.app
codesign --verify --deep --strict --verbose=2 ~/Desktop/Shotcut.app
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
