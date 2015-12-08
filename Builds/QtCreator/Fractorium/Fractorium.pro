TEMPLATE = app
QT += core gui opengl concurrent

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = fractorium

include(../defaults.pri)

PRJ_DIR = $$SRC_DIR/Fractorium

target.path = $$BIN_INSTALL_DIR
INSTALLS += target

assets.path = $$SHARE_INSTALL_DIR
assets.files = $$ASSETS_DIR/flam3-palettes.xml $$ASSETS_DIR/dark.qss
INSTALLS += assets

# FIXME: move to Data and install to $$SHARE_INSTALL_DIR ?
macx:ICON = ../../package/osx/Fractorium.icns

LIBS += -L$$absolute_path($$DESTDIR) -lEmber
LIBS += -L$$absolute_path($$DESTDIR) -lEmberCL

INCLUDEPATH += $$PRJ_DIR

!macx:PRECOMPILED_HEADER = $$PRJ_DIR/FractoriumPch.h

SOURCES += \
    $$PRJ_DIR/AboutDialog.cpp \
    $$PRJ_DIR/csshighlighter.cpp \
    $$PRJ_DIR/CurvesGraphicsView.cpp \
    $$PRJ_DIR/DoubleSpinBox.cpp \
    $$PRJ_DIR/FinalRenderDialog.cpp \
    $$PRJ_DIR/FinalRenderEmberController.cpp \
    $$PRJ_DIR/Fractorium.cpp \
    $$PRJ_DIR/FractoriumEmberController.cpp \
    $$PRJ_DIR/FractoriumInfo.cpp \
    $$PRJ_DIR/FractoriumLibrary.cpp \
    $$PRJ_DIR/FractoriumMenus.cpp \
    $$PRJ_DIR/FractoriumPalette.cpp \
    $$PRJ_DIR/FractoriumParams.cpp \
    $$PRJ_DIR/FractoriumPch.cpp \
    $$PRJ_DIR/FractoriumRender.cpp \
    $$PRJ_DIR/FractoriumSettings.cpp \
    $$PRJ_DIR/FractoriumToolbar.cpp \
    $$PRJ_DIR/FractoriumXaos.cpp \
    $$PRJ_DIR/FractoriumXformsAffine.cpp \
    $$PRJ_DIR/FractoriumXformsColor.cpp \
    $$PRJ_DIR/FractoriumXforms.cpp \
    $$PRJ_DIR/FractoriumXformsSelect.cpp \
    $$PRJ_DIR/FractoriumXformsVariations.cpp \
    $$PRJ_DIR/GLEmberController.cpp \
    $$PRJ_DIR/GLWidget.cpp \
    $$PRJ_DIR/Main.cpp \
    $$PRJ_DIR/OptionsDialog.cpp \
    $$PRJ_DIR/qcssparser.cpp \
    $$PRJ_DIR/qcssscanner.cpp \
    $$PRJ_DIR/QssDialog.cpp \
    $$PRJ_DIR/QssTextEdit.cpp \
    $$PRJ_DIR/SpinBox.cpp \
    $$PRJ_DIR/VariationsDialog.cpp

HEADERS += \
    $$SRC_COMMON_DIR/EmberCommon.h \
    $$SRC_COMMON_DIR/EmberCommonPch.h \
    $$SRC_COMMON_DIR/JpegUtils.h \
    $$PRJ_DIR/AboutDialog.h \
    $$PRJ_DIR/csshighlighter.h \
    $$PRJ_DIR/CurvesGraphicsView.h \
    $$PRJ_DIR/DoubleSpinBox.h \
    $$PRJ_DIR/DoubleSpinBoxTableItemDelegate.h \
    $$PRJ_DIR/EmberFile.h \
    $$PRJ_DIR/EmberTreeWidgetItem.h \
    $$PRJ_DIR/FinalRenderDialog.h \
    $$PRJ_DIR/FinalRenderEmberController.h \
    $$PRJ_DIR/FractoriumCommon.h \
    $$PRJ_DIR/FractoriumEmberController.h \
    $$PRJ_DIR/Fractorium.h \
    $$PRJ_DIR/FractoriumPch.h \
    $$PRJ_DIR/FractoriumSettings.h \
    $$PRJ_DIR/GLEmberController.h \
    $$PRJ_DIR/GLWidget.h \
    $$PRJ_DIR/OptionsDialog.h \
    $$PRJ_DIR/PaletteTableWidgetItem.h \
    $$PRJ_DIR/qcssparser.h \
    $$PRJ_DIR/qcssscanner.h \
    $$PRJ_DIR/qfunctions.h \
    $$PRJ_DIR/QssDialog.h \
    $$PRJ_DIR/QssTextEdit.h \
    $$PRJ_DIR/resource.h \
    $$PRJ_DIR/SpinBox.h \
    $$PRJ_DIR/StealthComboBox.h \
    $$PRJ_DIR/TableWidget.h \
    $$PRJ_DIR/TwoButtonComboWidget.h \
    $$PRJ_DIR/VariationsDialog.h \
    $$PRJ_DIR/VariationTreeWidgetItem.h

FORMS += \
    $$PRJ_DIR/AboutDialog.ui \
    $$PRJ_DIR/FinalRenderDialog.ui \
    $$PRJ_DIR/Fractorium.ui \
    $$PRJ_DIR/OptionsDialog.ui \
    $$PRJ_DIR/QssDialog.ui \
    $$PRJ_DIR/VariationsDialog.ui

OTHER_FILES += \
    $$PRJ_DIR/Fractorium.aps \
    $$PRJ_DIR/Fractorium.rc

RESOURCES += \
    $$PRJ_DIR/Fractorium.qrc

