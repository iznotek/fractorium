TEMPLATE = lib
CONFIG += plugin
CONFIG += shared
CONFIG -= app_bundle
CONFIG -= qt

include(../shared_settings.pri)

LIBS += -L$$absolute_path($$DESTDIR) -lEmber

!macx:PRECOMPILED_HEADER = ../../../Source/EmberCL/EmberCLPch.h

QMAKE_CXXFLAGS += -D_USRDLL 
QMAKE_CXXFLAGS += -D_CONSOLE
QMAKE_CXXFLAGS += -BUILDING_EMBERCL

SOURCES += \
	../../../Source/EmberCL/DllMain.cpp \
	../../../Source/EmberCL/DEOpenCLKernelCreator.cpp \
	../../../Source/EmberCL/FinalAccumOpenCLKernelCreator.cpp \
	../../../Source/EmberCL/FunctionMapper.cpp \
	../../../Source/EmberCL/IterOpenCLKernelCreator.cpp \
	../../../Source/EmberCL/OpenCLInfo.cpp \
	../../../Source/EmberCL/OpenCLWrapper.cpp \
	../../../Source/EmberCL/RendererCL.cpp \
	../../../Source/EmberCL/RendererCLDevice.cpp

include(deployment.pri)
qtcAddDeployment()

HEADERS += \
	../../../Source/EmberCL/DEOpenCLKernelCreator.h \
	../../../Source/EmberCL/EmberCLFunctions.h \
	../../../Source/EmberCL/EmberCLPch.h \
	../../../Source/EmberCL/EmberCLStructs.h \
	../../../Source/EmberCL/FinalAccumOpenCLKernelCreator.h \
	../../../Source/EmberCL/FunctionMapper.h \
	../../../Source/EmberCL/IterOpenCLKernelCreator.h \
	../../../Source/EmberCL/OpenCLInfo.h \
	../../../Source/EmberCL/OpenCLWrapper.h \
	../../../Source/EmberCL/RendererCL.h \
	../../../Source/EmberCL/RendererCLDevice.h

