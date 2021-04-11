#!/bin/sh
VERSION="$1"
sudo xcode-select -s /Applications/Xcode.app/

find ~/Desktop/Shotcut.app/Contents/Frameworks -type f -exec codesign --options=runtime -v -s Meltytech {} \;
find ~/Desktop/Shotcut.app/Contents/PlugIns -type f -exec codesign --options=runtime -v -s Meltytech {} \;
find ~/Desktop/Shotcut.app/Contents/Resources -type f -exec codesign --options=runtime -v -s Meltytech {} \;
xattr -cr ~/Desktop/Shotcut.app
codesign --options=runtime -v -s Meltytech \
  --entitlements ~/Projects/Shotcut/src/shotcut/scripts/notarization.entitlements \
  ~/Desktop/Shotcut.app/Contents/MacOS/{melt,ffmpeg,ffplay,ffprobe}
codesign --options=runtime -v -s Meltytech \
  --entitlements ~/Projects/Shotcut/src/shotcut/scripts/notarization.entitlements \
  ~/Desktop/Shotcut.app
codesign --verify --deep --strict --verbose=2 ~/Desktop/Shotcut.app
spctl -a -t exec -vv ~/Desktop/Shotcut.app

rm -rf ~/tmp/*
mv ~/Desktop/Shotcut.app ~/tmp
ln -s /Applications ~/tmp
cp ~/Projects/Shotcut/src/shotcut/COPYING ~/tmp
rm ~/Desktop/shotcut-macos-${VERSION}.dmg
hdiutil create -srcfolder ~/tmp -volname Shotcut -format UDBZ -size 300m \
  ~/Desktop/shotcut-macos-${VERSION}.dmg

./notarize.sh ~/Desktop/shotcut-macos-${VERSION}.dmg

echo wait for the e-mail from Apple and then run:
echo ./staple.sh ~/Desktop/shotcut-macos-${VERSION}.dmg
echo sudo xcode-select -s /Library/Developer/CommandLineTools
