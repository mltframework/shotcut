#!/bin/sh

fixlibs()
{
  target=$(dirname "$1")/$(basename "$1")
  echo fixlibs $target
  libs=$(otool -L "$target" | awk '/^\t\/opt\/local/{print $1}')

  # if the target is a lib, change its id
  #if [ $(echo "$1" | grep '\.dylib$') ] || [ $(echo "$1" | grep '\.so$') ]; then
  #  echo install_name_tool -id "@executable_path/lib/$(basename "$1")" "$target"
  #  install_name_tool -id "@executable_path/lib/$(basename "$1")" "$target"
  #fi

  for lib in $libs; do
    if [ $(basename "$lib") != $(basename "$target") ]; then
      newlib=$(basename "$lib")

      echo cp -n "$lib" lib/
      cp -n "$lib" lib/

      echo install_name_tool -change "$lib" "@executable_path/lib/$newlib" "$target"
      install_name_tool -change "$lib" "@executable_path/lib/$newlib" "$target"
    fi
  done

  for lib in $libs; do
    if [ $(basename "$lib") != $(basename "$target") ]; then
      newlib=$(basename "$lib")
      echo
      fixlibs "lib/$newlib"
    fi
  done
}

BUILD_DIR="../shotcut-build-desktop/Shotcut.app/Contents"

# copy qt_menu.nib
# try MacPorts first
if [ -d "/opt/local/lib/Resources/qt_menu.nib" ]; then
  echo cp -Rn /opt/local/lib/Resources/qt_menu.nib "$BUILD_DIR/Resources/"
  cp -Rn /opt/local/lib/Resources/qt_menu.nib "$BUILD_DIR/Resources/"
# try Qt Creator after that
elif [ -d "/Applications/Qt Creator.app/Contents/Frameworks/QtGui.framework/Resources/qt_menu.nib" ]; then
  echo cp -Rn "/Applications/Qt Creator.app/Contents/Frameworks/QtGui.framework/Resources/qt_menu.nib" "$BUILD_DIR/Resources/"
  cp -Rn "/Applications/Qt Creator.app/Contents/Frameworks/QtGui.framework/Resources/qt_menu.nib" "$BUILD_DIR/Resources/"
fi

cd "$BUILD_DIR/MacOS"

mkdir lib 2>/dev/null
fixlibs Shotcut

# MLT plugins
mkdir -p lib/mlt 2>/dev/null
echo cp ~/src/mlt/src/modules/libmlt*.dylib lib/mlt
cp ~/src/mlt/src/modules/libmlt*.dylib lib/mlt
mkdir share 2>/dev/null
echo cp -R /opt/local/share/mlt share
cp -R /opt/local/share/mlt share
for lib in lib/mlt/*; do
  fixlibs "$lib"
done

# Qt4 plugins
mkdir -p lib/qt4 2>/dev/null
# try MacPorts first
if [ -d "/opt/local/share/qt4/plugins" ]; then
  echo cp -Rn "/opt/local/share/qt4/plugins"/{accessible,bearer,codecs,designer,graphicssystems,iconengines,imageformats,qmltooling,sceneformats,script,sqldrivers} lib/qt4
  cp -Rn "/opt/local/share/qt4/plugins"/{accessible,bearer,codecs,designer,graphicssystems,iconengines,imageformats,qmltooling,sceneformats,script,sqldrivers} lib/qt4
# try Qt Creator next
elif [ -d "/Applications/Qt Creator.app/Contents/PlugIns" ]; then
  echo cp -Rn "/Applications/Qt Creator.app/Contents/PlugIns"/{accessible,bearer,codecs,designer,graphicssystems,iconengines,imageformats,qmltooling,sceneformats,script,sqldrivers} lib/qt4
  cp -Rn "/Applications/Qt Creator.app/Contents/PlugIns"/{accessible,bearer,codecs,designer,graphicssystems,iconengines,imageformats,qmltooling,sceneformats,script,sqldrivers} lib/qt4
fi
for dir in lib/qt4/*; do
  for lib in $dir/*; do
    fixlibs "$lib"
  done
done

# frei0r plugins
mkdir lib 2>/dev/null
cp -Rn /opt/local/lib/frei0r-1 lib
for lib in lib/frei0r-1/*; do
  fixlibs "$lib"
done

# LADSPA plugins
mkdir lib/ladspa 2>/dev/null
cp -Rn /opt/ladspa/lib/ladspa/* lib/ladspa
for lib in lib/ladspa/*; do
  fixlibs "$lib"
done

