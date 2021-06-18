include(shotcut.pri)

!minQtVersion(5, 12, 0) {
    message("Cannot build Shotcut with Qt version $${QT_VERSION}.")
    error("Use at least Qt 5.12.0.")
}

TEMPLATE = subdirs
SUBDIRS = CuteLogger src translations
cache()
src.depends = CuteLogger

codespell.target = codespell
codespell.commands = codespell -w -q 3 \
    -L shotcut,uint,seeked \
    -S export-edl.js,CuteLogger,drmingw,node_modules,moc_*,index.theme,*.ts,three.min.js,three.js,jquery.js,rangy-*.js,Makefile,shotcut.pro,*.desktop
codespell.depends = FORCE
QMAKE_EXTRA_TARGETS += codespell
