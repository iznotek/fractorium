#!/bin/bash

DIR=$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )

FRACTORIUM_ROOT=$DIR/../../..
PACKAGE=$FRACTORIUM_ROOT/release/Fractorium.app
export QMAKE=~/Qt/5.4.2-static/qtbase/bin/qmake

# Uncomment to force rebuild of package every time
# Probably a good idea in future
rm -rf "$FRACTORIUM_ROOT/release"
$FRACTORIUM_ROOT/Builds/QtCreator/build_all.sh --rebuild

cp $FRACTORIUM_ROOT/Data/flam3-palettes.xml $PACKAGE/Contents/MacOS/

export DYLD_LIBRARY_PATH=/usr/lib:/usr/local/lib:$FRACTORIUM_ROOT/release

cp /usr/lib/libxml2* $PACKAGE/Contents/libs
install_name_tool -id @executable_path/../libs/libxml2.2.dylib $PACKAGE/Contents/libs/libxml2.2.dylib
install_name_tool -change /usr/lib/libxml2.2.dylib  @executable_path/../libs/libxml2.2.dylib $PACKAGE/Contents/MacOS/Fractorium

dylibbundler -od -b -x $PACKAGE/Contents/MacOS/Fractorium -d $PACKAGE/Contents/libs

for i in EmberAnimate EmberGenome EmberRender
do
  cp $FRACTORIUM_ROOT/release/$i $PACKAGE/Contents/MacOS/
  dylibbundler -od -b -x $PACKAGE/Contents/MacOS/$i -d $PACKAGE/Contents/libs
  install_name_tool -change /usr/lib/libxml2.2.dylib  @executable_path/../libs/libxml2.2.dylib $PACKAGE/Contents/MacOS/$i
done

