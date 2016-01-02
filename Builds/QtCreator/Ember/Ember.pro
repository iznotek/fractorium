TEMPLATE = lib
CONFIG += plugin
CONFIG += shared
CONFIG -= app_bundle
CONFIG -= qt

include(../defaults.pri)

PRJ_DIR = $$SRC_DIR/Ember

target.path = $$LIB_INSTALL_DIR
INSTALLS += target

!macx:PRECOMPILED_HEADER = $$PRJ_DIR/EmberPch.h

QMAKE_CXXFLAGS += -D_USRDLL
QMAKE_CXXFLAGS += -D_CONSOLE
QMAKE_CXXFLAGS += -BUILDING_EMBER

SOURCES += \
    $$PRJ_DIR/Affine2D.cpp \
    $$PRJ_DIR/DllMain.cpp \
    $$PRJ_DIR/Ember.cpp \
    $$PRJ_DIR/EmberPch.cpp \
    $$PRJ_DIR/RendererBase.cpp \
    $$PRJ_DIR/Renderer.cpp

include(deployment.pri)
qtcAddDeployment()

HEADERS += \
    $$PRJ_DIR/Affine2D.h \
    $$PRJ_DIR/CarToRas.h \
    $$PRJ_DIR/Curves.h \
    $$PRJ_DIR/DensityFilter.h \
    $$PRJ_DIR/EmberDefines.h \
    $$PRJ_DIR/Ember.h \
    $$PRJ_DIR/EmberMotion.h \
    $$PRJ_DIR/EmberPch.h \
    $$PRJ_DIR/EmberToXml.h \
    $$PRJ_DIR/Interpolate.h \
    $$PRJ_DIR/Isaac.h \
    $$PRJ_DIR/Iterator.h \
    $$PRJ_DIR/Palette.h \
    $$PRJ_DIR/PaletteList.h \
    $$PRJ_DIR/Point.h \
    $$PRJ_DIR/RendererBase.h \
    $$PRJ_DIR/Renderer.h \
    $$PRJ_DIR/SheepTools.h \
    $$PRJ_DIR/SpatialFilter.h \
    $$PRJ_DIR/TemporalFilter.h \
    $$PRJ_DIR/Timing.h \
    $$PRJ_DIR/Utils.h \
    $$PRJ_DIR/Variation.h \
    $$PRJ_DIR/VariationList.h \
    $$PRJ_DIR/Variations01.h \
    $$PRJ_DIR/Variations02.h \
    $$PRJ_DIR/Variations03.h \
    $$PRJ_DIR/Variations04.h \
    $$PRJ_DIR/Variations05.h \
    $$PRJ_DIR/Variations06.h \
    $$PRJ_DIR/VariationsDC.h \
    $$PRJ_DIR/VarFuncs.h \
    $$PRJ_DIR/Xform.h \
    $$PRJ_DIR/XmlToEmber.h

