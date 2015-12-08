TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

TARGET = embergenome

include(../defaults.pri)

target.path = $$BIN_INSTALL_DIR
INSTALLS += target

LIBS += -L$$absolute_path($$DESTDIR) -lEmber
LIBS += -L$$absolute_path($$DESTDIR) -lEmberCL

PRJ_DIR = $$SRC_DIR/EmberGenome

!macx:PRECOMPILED_HEADER = $$SRC_COMMON_DIR/EmberCommonPch.h

SOURCES += \
    $$PRJ_DIR/EmberGenome.cpp \
    $$SRC_COMMON_DIR/EmberCommonPch.cpp

include(deployment.pri)
qtcAddDeployment()

HEADERS += \
    $$PRJ_DIR/EmberGenome.h \
    $$SRC_COMMON_DIR/EmberCommon.h \
    $$SRC_COMMON_DIR/EmberCommonPch.h \
    $$SRC_COMMON_DIR/EmberOptions.h \
    $$SRC_COMMON_DIR/JpegUtils.h \
    $$SRC_COMMON_DIR/SimpleGlob.h \
    $$SRC_COMMON_DIR/SimpleOpt.h

