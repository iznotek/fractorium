TEMPLATE = subdirs
CONFIG += ordered

LOCAL_LIB_DIR = $$(PWD)/Builds/lib
LOCAL_INCLUDE_DIR = $$(PWD)/Builds/include

unix {
  symlinks.commands = \
  test -d $$LOCAL_LIB_DIR || mkdir -p $$LOCAL_LIB_DIR ; \
  test -d $$LOCAL_INCLUDE_DIR || mkdir -p $$LOCAL_INCLUDE_DIR ; \
  test -e $$LOCAL_LIB_DIR/libOpenCL.so || \
    ln -s /usr/lib/x86_64-linux-gnu/libOpenCL.so.1 $$LOCAL_LIB_DIR/libOpenCL.so ; \
  test -e $$LOCAL_INCLUDE_DIR/GL || \
    ln -s /usr/include/nvidia-352/GL $$LOCAL_INCLUDE_DIR ; \
  test -e $$PWD/include/CL || \
    ln -s /usr/include/nvidia-352/CL $$LOCAL_INCLUDE_DIR ;
}

SUBDIRS += Builds/QtCreator/Ember Builds/QtCreator/EmberCL Builds/QtCreator/EmberAnimate Builds/QtCreator/EmberGenome Builds/QtCreator/EmberRender Builds/QtCreator/Fractorium

sub-Builds-QtCreator-Ember-make_first-ordered.depends = symlinks

QMAKE_EXTRA_TARGETS += sub-Builds-QtCreator-Ember-make_first-ordered symlinks
