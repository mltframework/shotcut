all: snap appimage
.PHONY: all snap appimage

snap:
	mkdir -p snap/gui
	cp ../../icons/snapflow-logo-64.png snap/gui
	cp org.snapflow.Snapflow.desktop snap/gui/snapflow.desktop
	sed -i 's|Icon=.*|Icon=$${SNAP}/meta/gui/snapflow-logo-64.png|' snap/gui/snapflow.desktop
	cat snap/gui/snapflow.desktop
	VERSION=$(shell wget -q "https://api.github.com/repos/mltframework/snapflow/releases/latest"  -O - | grep tag_name | cut -d'"' -f4 | sed 's|^v||')
	FILENAME="https://github.com/mltframework/snapflow/releases/download/v${VERSION}/snapflow-linux-x86_64-$(shell echo $VERSION | tr -d .).txz"
	sed "s/\$VERSION/${VERSION}/ ; s/\$FILENAME/${FILENAME}/" snapcraft.yaml.in > snap/snapcraft.yaml
	snapcraft --use-lxd

appimage: appimage/appimage.yml
	mkdir -p Snapflow
	echo $(shell wget -q "https://api.github.com/repos/mltframework/snapflow/releases/latest"  -O - | grep tag_name | cut -d'"' -f4 | sed 's|^v||') > Snapflow/VERSION
	wget -N https://raw.githubusercontent.com/probonopd/AppImages/master/pkg2appimage
	bash -ex pkg2appimage appimage/appimage.yml

clean:
	rm -rf out pkg2appimage Snapflow
