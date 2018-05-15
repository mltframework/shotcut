snap:
	snapcraft prime
	cp ../../../icons/shotcut-logo-64.png gui
	cp ../org.shotcut.Shotcut.desktop gui/shotcut.desktop
	sed -i 's|Icon=.*|Icon=${SNAP}/meta/gui/shotcut-logo-64.png|' gui/shotcut.desktop
	snapcraft snap

snap-clean:
	snapcraft clean
	rm -rf snap