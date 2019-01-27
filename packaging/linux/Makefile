snap:
	mkdir -p snap/gui
	cp ../../icons/shotcut-logo-64.png snap/gui
	cp org.shotcut.Shotcut.desktop snap/gui/shotcut.desktop
	sed -i 's|Icon=.*|Icon=$${SNAP}/meta/gui/shotcut-logo-64.png|' snap/gui/shotcut.desktop
	cat snap/gui/shotcut.desktop
	snapcraft cleanbuild

appimage: appimage/appimage.yml
	wget -N https://raw.githubusercontent.com/probonopd/AppImages/master/pkg2appimage
	bash -ex pkg2appimage appimage/appimage.yml

clean:
	rm -rf out pkg2appimage Shotcut
