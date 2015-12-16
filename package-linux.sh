#!/bin/bash

USAGE="`basename $0`
Run this script from the project root.

Without options, the default is to build a signed source package for uploading
to a Launchpad PPA.

Options:

-h  --help

--binary-only
--source-only
--source-and-binary
--signed
--unsigned"

# version for the debian package
VERSION=0.9.9.2d
PROJECT=fractorium
PROJECT_ROOT=$PWD
PPA_DIR="$HOME/PPA/$PROJECT-$VERSION"
TAR_NAME="$PROJECT-$VERSION.tar.gz"

if [ ! -d '.git' -o ! -f 'main.pro' ]; then
    echo "Run `basename $0` from the project root."
    exit 2
fi

# Set defaults.
OPT_BUILD_BINARY=0
OPT_BUILD_SOURCE=1
OPT_SIGNED=1

while [ $# -gt 0 ]; do
    case "$1" in
        --binary-only)       OPT_BUILD_SOURCE=0
                             OPT_BUILD_BINARY=1
                             ;;
        --source-only)       OPT_BUILD_SOURCE=1
                             OPT_BUILD_BINARY=0
                             ;;
        --source-and-binary) OPT_BUILD_SOURCE=1
                             OPT_BUILD_BINARY=1
                             ;;
        --signed)            OPT_SIGNED=1
                             ;;
        --unsigned)          OPT_SIGNED=0
                             ;;
        -h|--help) echo "$USAGE"
                   exit 0;;
    esac
    shift
done

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
    --exclude='debian' \
    --exclude='Bin' \
    --exclude-vcs \
    --exclude-vcs-ignores \
    --exclude-backups \
    -czf "$PPA_DIR/$TAR_NAME" .

[ $? -ne 0 ] && echo "Tar command failed." && exit 2

cd "$PPA_DIR"
bzr dh-make $PROJECT $VERSION $TAR_NAME

[ $? -ne 0 ] && echo "bzr dh-make command failed." && exit 2

rm "$PPA_DIR/fractorium/debian" -r
cp -R "$PROJECT_ROOT/debian" "$PPA_DIR/fractorium"

cd "$PPA_DIR/fractorium" &&\
    bzr add . &&\
    bzr commit -m "Debian package $VERSION"

[ $? -ne 0 ] && echo "bzr command failed." && exit 2

# Build a source package.

# Launchpad only needs a signed source package. It will build its own binary on
# the servers.

if [ $OPT_BUILD_SOURCE -eq 1 ]; then
    if [ $OPT_SIGNED -eq 1 ]; then
        bzr builddeb -- -S
    else
        bzr builddeb -- -S -us -uc
    fi
fi

[ $? -ne 0 ] && echo "bzr builddeb for source package failed." && exit 2

# Build an binary package.

if [ $OPT_BUILD_BINARY -eq 1 ]; then
    if [ $OPT_SIGNED -eq 1 ]; then
        bzr builddeb -- -b
    else
        bzr builddeb -- -b -us -uc
    fi
fi

[ $? -ne 0 ] && echo "bzr builddeb for source package failed." && exit 2


