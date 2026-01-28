find_package(Qt6 REQUIRED COMPONENTS LinguistTools)

# Translation howto:
# - Translations are mentioned in the LANGUAGES variable below
# - To update the .ts files, cd into translations and run 'ninja translations'
# - The .qm files are created as part of a regular make command

set(LANGUAGES
    ar
    ca
    cs
    da
    de
    el
    en
    en_GB
    es
    et
    eu
    fi
    fr
    fr_CA
    ga
    gd
    gl
    he_IL
    hu
    it
    ko
    ja
    lt
    nb
    nn
    ne
    nl
    oc
    pl
    pt_BR
    pt_PT
    ro
    ru
    sk
    sl
    sv
    th
    tr
    uk
    zh_CN
    zh-Hans
    zh_TW
)

list(TRANSFORM LANGUAGES PREPEND "shotcut_" OUTPUT_VARIABLE TS_FILES)
list(TRANSFORM TS_FILES APPEND ".qm" OUTPUT_VARIABLE QM_FILES)
list(TRANSFORM TS_FILES PREPEND ${CMAKE_CURRENT_SOURCE_DIR}/)
list(TRANSFORM TS_FILES APPEND ".ts")

# lupdate ts files
file(GLOB_RECURSE SOURCES "${CMAKE_SOURCE_DIR}/src/*")
qt_add_lupdate(shotcut TS_FILES ${TS_FILES} SOURCES ${SOURCES}
    OPTIONS -locations absolute -no-obsolete -recursive)
add_custom_target(translations DEPENDS shotcut_lupdate)

# generate qm files with lrelease
qt6_add_translation(QM_FILES ${TS_FILES})
add_custom_target(qm ALL DEPENDS ${QM_FILES})

if(UNIX AND NOT APPLE)
  include(GNUInstallDirs)
  install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/ DESTINATION ${CMAKE_INSTALL_DATADIR}/shotcut/translations
    FILES_MATCHING PATTERN "*.qm" PATTERN CMakeFiles EXCLUDE)
endif()
