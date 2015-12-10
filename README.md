Fractorium
==========

A Qt-based fractal flame editor which uses a C++ re-write of the flam3 algorithm
named Ember and a GPU capable version named EmberCL which implements a portion
of the cuburn algorithm in OpenCL.

# Download

Windows: TODO

Linux: TODO

Mac OS/X (10.9+): TODO

# Building from git

## Windows

TODO

## Linux

Install `git` and clone the repository:

```
sudo apt-get install git
git clone https://github.com/mfeemster/fractorium
```

Install the dependencies:

```
sudo apt-get install g++ libdbus-1-dev libgl1-mesa-dev libgl-dev libglm-dev libjpeg-dev libpng12-dev libtbb-dev libxml2-dev qt5-default qt5-qmake qtbase5-dev libqt5opengl5-dev opencl-headers nvidia-opencl-dev
```

Compile the binary:

```
cd fractorium
qmake
make
```

Run the binary from the release folder:

```
cd Bin/release
./fractorium
```

`sudo make install` will install the files directly. `sudo make uninstall` is
also available.

You can also compile a `.deb` package to install locally.

If you want the easiest way, use the `checkinstall` tool:

```
sudo apt-get install checkinstall
cd fractorium
sudo checkinstall make install
```

It is not difficult to compile a `.deb` package in the regular way though. A few
more tools will be necessary:

```
sudo apt-get install bzr bzr-builddeb dh-make debhelper
```

A helper script is available, use `package-linux.sh` in the project root. It
will create `~/PPA/fractorium-VERSION` as a work folder, by default it builds a
signed source package.

For local use you probably want an unsigned binary package:

```
cd fractorium
./package-linux.sh --binary-only --unsigned
```

## Mac OS/X 10.9+

Install Xcode from the App Store. Install [homebrew](http://brew.sh/).

Install `git` and clone the repository:

```
brew install git
git clone https://github.com/mfeemster/fractorium
```

Install the dependencies:

```
brew install qt5 tbb glm dbus jpeg libpng glib libxml2
```

TODO: Confirm if `glib` and `libxml2` are actually needed.

Add the Qt `bin` folder to `PATH` to make `qmake` available. In
`~/.bash_profile` or `~/.bashrc`:

```
PATH=/usr/local/opt/qt5/bin:$PATH
export PATH
```

Compile the binary:

```
cd fractorium
qmake CONFIG-=app_bundle
make
```

Run the binary from the release folder:

```
cd Bin/release
./fractorium
```

# OpenCL tips

Nvidia, Ati, Intel.

## Windows

TODO

## Linux

TODO

## Mac OS/X

TODO
