snap:
	mkdir -p snap/meta/gui
	cp ../../icons/shotcut-logo-64.png snap/gui
	cp org.shotcut.Shotcut.desktop snap/gui/shotcut.desktop
	sed -i 's|Icon=.*|Icon=${SNAP}/meta/gui/shotcut-logo-64.png|' snap/gui/shotcut.desktop
	snapcraft cleanbuild
