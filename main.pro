TEMPLATE = subdirs
CONFIG += ordered
CONFIG += autobuild

# message(PWD: $$(PWD))

LOCAL_LIB_DIR = $$(PWD)/Builds/lib
LOCAL_INCLUDE_DIR = $$(PWD)/Builds/include

unix:symlinks.commands = $$(PWD)/Builds/create-symlinks.sh \"$$LOCAL_LIB_DIR\" \"$$LOCAL_INCLUDE_DIR\"

SUBDIRS += Builds/QtCreator/Ember Builds/QtCreator/EmberCL Builds/QtCreator/EmberAnimate Builds/QtCreator/EmberGenome Builds/QtCreator/EmberRender Builds/QtCreator/Fractorium

sub-Builds-QtCreator-Ember-make_first-ordered.depends = symlinks

QMAKE_EXTRA_TARGETS += sub-Builds-QtCreator-Ember-make_first-ordered symlinks
