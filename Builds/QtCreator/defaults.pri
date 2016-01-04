VERSION = 0.9.9.2
win32:CONFIG += skip_target_version_ext
message(PWD: $$(PWD))

# TODO: win32 install dirs?

unix|macx {
  LIB_INSTALL_DIR = /usr/lib
  BIN_INSTALL_DIR = /usr/bin
  SHARE_INSTALL_DIR = /usr/share/fractorium
}

# When loaded by QtCreator
EMBER_ROOT = $$(PWD)/../../..

# When compiling from project root
autobuild {
  EMBER_ROOT = $$(PWD)/../..
}

win32:{
  EMBER_ROOT = $$(PWD)../../..
  EXTERNAL_DIR = $$(PWD)../../../../External
  EXTERNAL_LIB = $$(PWD)../../../External/libs
# EXTERNAL_DIR which contains Third Party Codes is in the parent folder of "fractorium"
# EXTERNAL_LIB is in EXTERNAL_DIR actually, but it is strange that EXTERNAL_DIR must go
# one more step upper than EXTERNAL_LIB to get it work
  LIB_INSTALL_DIR = $$(PWD)../../../Install/lib
  BIN_INSTALL_DIR = $$(PWD)../../../Install/bin
  SHARE_INSTALL_DIR = $$(PWD)../../../Install/share/fractorium
# INSTALL_DIRs Don't work?
  message(EMBER_ROOT: $$absolute_path($$EMBER_ROOT))
  message(EXTERNAL: $$absolute_path($$EXTERNAL_DIR))
  message(EXTERNAL_LIB:$$absolute_path($$EXTERNAL_LIB) )
}

message(EMBER_ROOT: $$EMBER_ROOT)

SRC_DIR = $$EMBER_ROOT/Source
SRC_COMMON_DIR = $$EMBER_ROOT/Source/EmberCommon
ASSETS_DIR = $$EMBER_ROOT/Data
LOCAL_LIB_DIR = $$(PWD)/../../lib
LOCAL_INCLUDE_DIR = $$(PWD)/../../include

win32:{
  LOCAL_LIB_DIR = $$(PWD)../../lib
  LOCAL_INCLUDE_DIR = $$(PWD)../../include
}
CONFIG(release, debug|release) {
  CONFIG += warn_off
  DESTDIR = $$(PWD)/../../../Bin/release
  win32:DESTDIR = $$(PWD)../../../Bin/release
}

CONFIG(debug, debug|release) {
  DESTDIR = $$(PWD)/../../../Bin/debug
  win32:DESTDIR = $$(PWD)../../../Bin/debug
}

macx {
  LIBS += -framework OpenGL
  LIBS += -framework OpenCL

  # homebrew installs into /usr/local
  LIBS += -L/usr/local/lib

  INCLUDEPATH += /usr/local/include

  QMAKE_MAC_SDK = macosx10.11
  QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.9
  
  QMAKE_CXXFLAGS += -mmacosx-version-min=10.9 -arch x86_64
  QMAKE_CXXFLAGS += -stdlib=libc++
}

!macx:!win32 {
  CONFIG += precompile_header

  LIBS += -L/usr/lib/x86_64-linux-gnu -L$$LOCAL_LIB_DIR -lGL
  LIBS += -L/usr/lib/x86_64-linux-gnu -L$$LOCAL_LIB_DIR -lOpenCL

  QMAKE_LFLAGS_RELEASE += -s
}
# Win32: put "GlU32.Lib" "WS2_32.Lib" "OpenGL32.Lib" under "MSVC2013\Windows Kits\8.1\Lib\winv6.3\um\x64\" ,
# and "CUDA\v7.5\lib\x64\OpenCL.lib" into $$LOCAL_LIB_DIR
win32 {

  LIBS +=$$absolute_path($$EXTERNAL_LIB)/GlU32.Lib
  LIBS +=$$absolute_path($$EXTERNAL_LIB)/OpenGL32.lib
  LIBS +=$$absolute_path($$EXTERNAL_LIB)/WS2_32.lib
  LIBS +=$$absolute_path($$EXTERNAL_LIB)/OpenCL.lib
}
!win32 {
  native {
    QMAKE_CXXFLAGS += -march=native
  } else {
    QMAKE_CXXFLAGS += -march=k8
  }
}

OBJECTS_DIR = $$PWD/.obj
MOC_DIR = $$PWD/.moc
RCC_DIR = $$PWD/.qrc
UI_DIR = $$PWD/.ui
!win32 {
  LIBS += -L/usr/lib -ljpeg
  LIBS += -L/usr/lib -lpng
  LIBS += -L/usr/lib -ltbb
  LIBS += -L/usr/lib -lpthread
  LIBS += -L/usr/lib/x86_64-linux-gnu -lxml2

  CMAKE_CXXFLAGS += -DCL_USE_DEPRECATED_OPENCL_1_1_APIS

# NOTE: last path will be the first to search. gcc -I and -L appends to the
# beginning of the path list.

# NOTE: qmake will resolve symlinks. If /usr/local/include/CL is a symlink to
# /usr/include/nvidia-352/CL, qmake will generate Makefiles using the latter.

  INCLUDEPATH += /usr/include
  INCLUDEPATH += /usr/local/include
}

INCLUDEPATH += $$LOCAL_INCLUDE_DIR/vendor
INCLUDEPATH += $$LOCAL_INCLUDE_DIR

# Using a local version of opencl-headers, to make sure version 1.2.
#INCLUDEPATH += /usr/include/CL
#INCLUDEPATH += /usr/local/include/CL
!win32 {
  INCLUDEPATH += /usr/include/GL
  INCLUDEPATH += /usr/local/include/GL

  INCLUDEPATH += /usr/include/glm
  INCLUDEPATH += /usr/include/tbb
  INCLUDEPATH += /usr/include/libxml2
}
win32 {
  INCLUDEPATH += $$EXTERNAL_DIR/glm
  INCLUDEPATH += $$EXTERNAL_DIR/tbb/include
  INCLUDEPATH += $$EXTERNAL_DIR/libjpeg
  INCLUDEPATH += $$EXTERNAL_DIR/libpng
  INCLUDEPATH += $$EXTERNAL_DIR/libxml2/include
  LIBS += $$absolute_path($$EXTERNAL_LIB)/libjpeg.lib
  LIBS += $$absolute_path($$EXTERNAL_LIB)/libpng.lib /NODEFAULTLIB:LIBCMT
  LIBS += $$absolute_path($$EXTERNAL_LIB)/libxml2.lib
  LIBS += $$absolute_path($$EXTERNAL_LIB)/tbb.lib
  LIBS += $$absolute_path($$EXTERNAL_LIB)/tbb_debug.lib
  LIBS += $$absolute_path($$EXTERNAL_LIB)/zlib.lib
}
INCLUDEPATH += $$SRC_DIR/Ember
INCLUDEPATH += $$SRC_DIR/EmberCL
INCLUDEPATH += $$SRC_DIR/EmberCommon

QMAKE_CXXFLAGS_RELEASE += -O2
QMAKE_CXXFLAGS_RELEASE += -DNDEBUG
!win32 {
  QMAKE_CXXFLAGS_RELEASE += -fomit-frame-pointer
  QMAKE_CXXFLAGS += -fPIC
  QMAKE_CXXFLAGS += -fpermissive
  QMAKE_CXXFLAGS += -pedantic
  QMAKE_CXXFLAGS += -std=c++11
  QMAKE_CXXFLAGS += -Wnon-virtual-dtor
  QMAKE_CXXFLAGS += -Wshadow
  QMAKE_CXXFLAGS += -Winit-self
  QMAKE_CXXFLAGS += -Wredundant-decls
  QMAKE_CXXFLAGS += -Wcast-align
  QMAKE_CXXFLAGS += -Winline
  QMAKE_CXXFLAGS += -Wunreachable-code
  QMAKE_CXXFLAGS += -Wmissing-include-dirs
  QMAKE_CXXFLAGS += -Wswitch-enum
  QMAKE_CXXFLAGS += -Wswitch-default
  QMAKE_CXXFLAGS += -Wmain
  QMAKE_CXXFLAGS += -Wzero-as-null-pointer-constant
  QMAKE_CXXFLAGS += -Wfatal-errors
  QMAKE_CXXFLAGS += -Wall -fpermissive
  QMAKE_CXXFLAGS += -Wold-style-cast
  QMAKE_CXXFLAGS += -Wno-unused-parameter
  QMAKE_CXXFLAGS += -Wno-unused-function
  QMAKE_CXXFLAGS += -Wold-style-cast
}
QMAKE_CXXFLAGS += -D_M_X64
QMAKE_CXXFLAGS += -D_CONSOLE
QMAKE_CXXFLAGS += -D_USRDLL
win32:QMAKE_CXXFLAGS += -bigobj 
