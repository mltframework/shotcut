snap:
	snapcraft prime
	cp ../../../icons/shotcut-logo-64.png gui
	cp ../org.shotcut.Shotcut.desktop gui/shotcut.desktop
	desktop-file-edit --set-key=Icon --set-value='${SNAP}/meta/gui/shotcut-logo-64.png' gui/shotcut.desktop
	snapcraft snap

snap-clean:
	snapcraft clean
	rm -rf snap