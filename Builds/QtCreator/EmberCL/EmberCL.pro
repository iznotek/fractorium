TEMPLATE = lib
CONFIG += plugin
CONFIG += shared
CONFIG -= app_bundle
CONFIG -= qt

include(../defaults.pri)

PRJ_DIR = $$SRC_DIR/EmberCL

target.path = $$LIB_INSTALL_DIR
INSTALLS += target

LIBS += -L$$absolute_path($$DESTDIR) -lEmber

!macx:PRECOMPILED_HEADER = $$PRJ_DIR/EmberCLPch.h

QMAKE_CXXFLAGS += -D_USRDLL
QMAKE_CXXFLAGS += -D_CONSOLE
QMAKE_CXXFLAGS += -BUILDING_EMBERCL
win32: DEFINES +=BUILDING_EMBERCL

SOURCES += \
    $$PRJ_DIR/DEOpenCLKernelCreator.cpp \
    $$PRJ_DIR/DllMain.cpp \
    $$PRJ_DIR/FinalAccumOpenCLKernelCreator.cpp \
    $$PRJ_DIR/FunctionMapper.cpp \
    $$PRJ_DIR/IterOpenCLKernelCreator.cpp \
    $$PRJ_DIR/OpenCLInfo.cpp \
    $$PRJ_DIR/OpenCLWrapper.cpp \
    $$PRJ_DIR/RendererCL.cpp \
    $$PRJ_DIR/RendererClDevice.cpp

include(deployment.pri)
qtcAddDeployment()

HEADERS += \
    $$PRJ_DIR/DEOpenCLKernelCreator.h \
    $$PRJ_DIR/EmberCLFunctions.h \
    $$PRJ_DIR/EmberCLPch.h \
    $$PRJ_DIR/EmberCLStructs.h \
    $$PRJ_DIR/FinalAccumOpenCLKernelCreator.h \
    $$PRJ_DIR/FunctionMapper.h \
    $$PRJ_DIR/IterOpenCLKernelCreator.h \
    $$PRJ_DIR/OpenCLInfo.h \
    $$PRJ_DIR/OpenCLWrapper.h \
    $$PRJ_DIR/RendererClDevice.h \
    $$PRJ_DIR/RendererCL.h

