# copied from https://raw.githubusercontent.com/bjorn/tiled/master/translations/translations.pro

# This file is largely based on the way translations are compiled in
# translations.pro from Qt Creator.
#
# Translation howto:
# - Translations are mentioned in the LANGUAGES variable below
# - To update the .ts files, cd into translations and run 'make ts'
# - The .qm files are created as part of a regular make command
#

# The list of supported translations
LANGUAGES = \
    ar \
    ca \
    cs \
    da \
    de \
    el \
    en \
    en_GB \
    es \
    et \
    fi \
    fr \
    gd \
    gl \
    hu \
    it \
    ko \
    ja \
    nb \
    nn \
    ne \
    nl \
    oc \
    pl \
    pt_BR \
    pt_PT \
    ru \
    sk \
    sl \
    sv \
    th \
    tr \
    uk \
    zh_CN \
    zh_TW

# Helper function prepending and appending text to all values
# Usage: var, prepend, append
defineReplace(prependAppend) {
    for(a,$$1):result += $$2$${a}$$3
    return($$result)
}

# Large hack to make sure this pro file does not try to compile an application
TEMPLATE = app
TARGET = phony_target
CONFIG -= qt sdk separate_debug_info gdb_dwarf_index
QT =
LIBS =
QMAKE_LINK = @: IGNORE THIS LINE
OBJECTS_DIR =
win32:CONFIG -= embed_manifest_exe

TRANSLATIONS = $$prependAppend(LANGUAGES, $$PWD/shotcut_, .ts)
LUPDATE = $$[QT_INSTALL_BINS]/lupdate -locations absolute -no-obsolete
LRELEASE = $$QMAKE_LRELEASE
isEmpty(LRELEASE):LRELEASE = $$[QT_INSTALL_BINS]/lrelease

ts.commands = cd $$PWD/.. && $$LUPDATE src -ts $$TRANSLATIONS
QMAKE_EXTRA_TARGETS += ts

win32 {
    TARGET_DIR = .
} else:macx {
    TARGET_DIR = ../src/Shotcut.app/Contents/Resources/translations
} else {
    TARGET_DIR = ../share/shotcut/translations
}

updateqm.input = TRANSLATIONS
updateqm.output = $$OUT_PWD/$$TARGET_DIR/${QMAKE_FILE_BASE}.qm
isEmpty(vcproj):updateqm.variable_out = PRE_TARGETDEPS
updateqm.commands = $$LRELEASE ${QMAKE_FILE_IN} -qm ${QMAKE_FILE_OUT}
updateqm.name = LRELEASE ${QMAKE_FILE_IN}
updateqm.CONFIG += no_link
QMAKE_EXTRA_COMPILERS += updateqm

unix:!mac {
    # Install rule for translations
    qmfiles.files = $$prependAppend(LANGUAGES, $$OUT_PWD/$$TARGET_DIR/shotcut_, .qm)
    qmfiles.path = $${PREFIX}/share/shotcut/translations
    qmfiles.CONFIG += no_check_exist
    INSTALLS += qmfiles
}
