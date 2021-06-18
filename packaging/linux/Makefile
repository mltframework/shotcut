all: snap appimage
.PHONY: all snap appimage

snap:
	mkdir -p snap/gui
	cp ../../icons/shotcut-logo-64.png snap/gui
	cp org.shotcut.Shotcut.desktop snap/gui/shotcut.desktop
	sed -i 's|Icon=.*|Icon=$${SNAP}/meta/gui/shotcut-logo-64.png|' snap/gui/shotcut.desktop
	cat snap/gui/shotcut.desktop
	VERSION=$(shell wget -q "https://api.github.com/repos/mltframework/shotcut/releases/latest"  -O - | grep tag_name | cut -d'"' -f4 | sed 's|^v||')
	FILENAME="https://github.com/mltframework/shotcut/releases/download/v${VERSION}/shotcut-linux-x86_64-$(shell echo $VERSION | tr -d .).txz"
	sed "s/\$VERSION/${VERSION}/ ; s/\$FILENAME/${FILENAME}/" snapcraft.yaml.in > snap/snapcraft.yaml
	snapcraft --use-lxd

appimage: appimage/appimage.yml
	mkdir -p Shotcut
	echo $(shell wget -q "https://api.github.com/repos/mltframework/shotcut/releases/latest"  -O - | grep tag_name | cut -d'"' -f4 | sed 's|^v||') > Shotcut/VERSION
	wget -N https://raw.githubusercontent.com/probonopd/AppImages/master/pkg2appimage
	bash -ex pkg2appimage appimage/appimage.yml

clean:
	rm -rf out pkg2appimage Shotcut
