TEMPLATE = subdirs
CONFIG += ordered

unix {
  symlinks.commands = \
  test -d $$PWD/lib || mkdir -p $$PWD/lib; \
  test -d $$PWD/include || mkdir -p $$PWD/include; \
  test -e $$PWD/lib/libOpenCL.so || \
    ln -s /usr/lib/x86_64-linux-gnu/libOpenCL.so.1 $$PWD/lib/libOpenCL.so; \
  test -e $$PWD/include/GL || \
    ln -s /usr/include/nvidia-352/GL $$PWD/include; \
  test -e $$PWD/include/CL || \
    ln -s /usr/include/nvidia-352/CL $$PWD/include;
}

SUBDIRS += Builds/QtCreator/Ember Builds/QtCreator/EmberCL Builds/QtCreator/EmberAnimate Builds/QtCreator/EmberGenome Builds/QtCreator/EmberRender Builds/QtCreator/Fractorium

sub-Builds-QtCreator-Ember-make_first-ordered.depends = symlinks

QMAKE_EXTRA_TARGETS += sub-Builds-QtCreator-Ember-make_first-ordered symlinks
