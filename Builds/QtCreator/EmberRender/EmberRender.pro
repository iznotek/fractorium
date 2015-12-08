TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

TARGET = emberrender

include(../defaults.pri)

PRJ_DIR = $$SRC_DIR/EmberRender

target.path = $$BIN_INSTALL_DIR
INSTALLS += target

LIBS += -L$$absolute_path($$DESTDIR) -lEmber
LIBS += -L$$absolute_path($$DESTDIR) -lEmberCL

!macx:PRECOMPILED_HEADER = $$SRC_COMMON_DIR/EmberCommonPch.h

SOURCES += \
    $$PRJ_DIR/EmberRender.cpp \
    $$SRC_COMMON_DIR/EmberCommonPch.cpp

include(deployment.pri)
qtcAddDeployment()

HEADERS += \
    $$PRJ_DIR/EmberRender.h \
    $$SRC_COMMON_DIR/EmberCommon.h \
    $$SRC_COMMON_DIR/EmberCommonPch.h \
    $$SRC_COMMON_DIR/EmberOptions.h \
    $$SRC_COMMON_DIR/JpegUtils.h \
    $$SRC_COMMON_DIR/SimpleGlob.h \
    $$SRC_COMMON_DIR/SimpleOpt.h

