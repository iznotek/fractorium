#!/bin/bash

# version for the debian package
VERSION=0.9.9.2b
PROJECT=fractorium
PROJECT_ROOT=$PWD
PPA_DIR="$HOME/PPA/$PROJECT-$VERSION"
TAR_NAME="$PROJECT-$VERSION.tar.gz"

if [ ! -d '.git' -o ! -f 'main.pro' ]; then
    echo "Run `basename $0` from the project root."
    exit 2
fi

tarversion=$(tar --version | head -1 | sed -e 's/tar (GNU tar) \+\([0-9\.]\+\)$/\1/; s/[^0-9]//g; s/^\(.{3}\).*$/\1/;')

if [[ "$tarversion" -lt "128" ]]; then
    echo "Tar >= 1.28 is required. Download the .deb from https://launchpad.net/ubuntu/+source/tar/ and install manually."
    exit 2
fi

if [ ! -d "$PPA_DIR" ]; then
    mkdir -p "$PPA_DIR"
else
    echo -n "PPA work folder already exists: $PPA_DIR
Move this folder aside or remove it.
"
    exit 2
fi

# tar 1.28 required for --exclude-vcs-ignores

# FIXME: somehow it didn't ignore the 'Bin' folder.

tar --exclude='package-linux.sh' \
    --exclude='Bin' \
    --exclude-vcs \
    --exclude-vcs-ignores \
    --exclude-backups \
    -czf "$PPA_DIR/$TAR_NAME" .

[ $? -ne 0 ] && echo "Tar command failed." && exit 2

# TODO: find the option to specify single binary, so the question can be skipped.

cd "$PPA_DIR" &&\
    bzr dh-make $PROJECT $VERSION $TAR_NAME &&\
    cd fractorium/debian &&\
    rm *.ex *.EX README.Debian README.source &&\
    cd ..

[ $? -ne 0 ] && echo "bzr dh-make command failed." && exit 2

bzr add . &&\
    bzr commit -m "Debian package $VERSION"

[ $? -ne 0 ] && echo "bzr command failed." && exit 2

# Build a signed source package for Launchpad, it will build its own binary on
# the servers.

bzr builddeb -- -S

[ $? -ne 0 ] && echo "bzr builddeb for source package failed." && exit 2

# Optionally, build an unsigned binary package for local use.

# bzr builddeb -- -b -us -uc
#
# [ $? -ne 0 ] && echo "bzr builddeb for source package failed." && exit 2

