#!/bin/sh
VERSION="$1"
sudo xcode-select -s /Applications/Xcode.app/

xattr -cr ~/Desktop/Shotcut.app
codesign --force --options=runtime -v -s Meltytech \
  --entitlements ~/Projects/Shotcut/src/shotcut/scripts/notarization.entitlements \
  ~/Desktop/Shotcut.app/Contents/MacOS/*
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
