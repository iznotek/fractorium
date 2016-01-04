#Building Guide for Fractorium Using MSVC2013 and Qt Creator (64 bit)
##Requirements

You need to have MSVC2013 compiler and  Qt for Windows 64-bit (VS 2013) like Qt 5.5.1 (http://download.qt.io/official_releases/qt/5.5/5.5.1/qt-opensource-windows-x86-msvc2013_64-5.5.1.exe)

##Prerequisites
Download Prerequisites ( /glm /libjpeg /libpng /libxml2 /tbb /zlib ) according to [mfeemster's wiki](https://github.com/mfeemster/fractorium/wiki/Building).  
Extract them in the parallel folder of `fractorium` named `External`  
You can refer to the folder structure below  
Open your `Visual Studio Tools Command Prompt (amd64 2013)` and do the followings in folders below:  

######\External\libjpeg (jpegsr9a.zip)
-   run in prompt  
    ```
    SET Include=%Include%;"C:\Program Files (x86)\Microsoft SDKs\Windows\v7.1A\Include"
    ```  
    (to include WIN32.mak)  
    (I use `SET Include=%Include%;"B:\MSVC2013\Windows Kits\v7.1A\Include"`)  
    ```
    nmake /f makefile.vc  setup-v10
    nmake nodebug=1 /f makefile.vc clean all
    ```  
    copy `libjpeg.lib` in `External\libjpeg` to `External\libs`


###### \External\zlib (zlib128.zip)
- run in prompt  
    ```
    nmake -f win32/Makefile.msc clean all
    ```
    copy `zlib.lib` in `External\zlib` to `External\libs`

###### \External\libxml2 (libxml2-2.9.3.zip)
- cd to `External\libxml2\win32`  
    ```
    cscript configure.js compiler=msvc iconv=no zlib=yes include=..\..\zlib lib=..\..\zlib
    nmake /f Makefile.msvc clean all
    ```  
    copy `libxml2.lib` in `External\libxml2\win32\bin.msvc` to `External\libs`

###### \External\libpng (lpng1620.zip)
- First, copy `zlib.lib` `zlib.h` and `zconf.h` to `External\libpng\zlib` (create this folder if no exsiting)  
    ```
    nmake -f  scripts\makefile.vcwin32 clean all
    ```   
    copy `libpng.lib` in `External\libpng` to `External\libs`

###### \External\glm (glm-0.9.6.3.zip)
- noting to do, make sure you extract it correctly.

###### \External\tbb (tbb44_20151115oss_win_0.zip)
- copy `External\tbb\lib\intel64\vc12\tbb_debug.lib` and `External\tbb\lib\intel64\vc12\tbb.lib` to `External\libs`

##Collect libs and includes
###### \External\libs
- copy `GlU32.Lib` `WS2_32.Lib` `OpenGL32.Lib` under `MSVC2013\Windows Kits\8.1\Lib\winv6.3\um\x64`
    to this folder (`\External\libs`)  
    install CUDA toolkit and copy `CUDA\v7.5\lib\x64\OpenCL.lib` to this folder (`\External\libs`)  
- Now you should already have these files in the folder:  
```
GlU32.Lib
libjpeg.lib
libpng.lib
libxml2.lib
OpenCL.lib
OpenGL32.Lib
tbb.lib
tbb_debug.lib
WS2_32.Lib
zlib.lib
```

###### \fractorium\Builds\lib
- copy anything(they are `*.h`) under `CUDA\v7.5\include\CL`
     to this folder `\fractorium\Builds\include\vendor\CL` (overwrite)

**All Done!**  

You shuold have file structure like this:
```
[YOUR ROOT FOLDER]
│  
├─External
│  ├─glm
│  │  ├─glm
│  │  ...
│  ├─libjpeg
│  ├─libpng
│  │  ├─scripts
│  │  ├─zlib
│  │  ...
│  ├─libs
│  │      GlU32.Lib
│  │      libjpeg.lib
│  │      libpng.lib
│  │      libxml2.lib
│  │      OpenCL.lib
│  │      OpenGL32.Lib
│  │      tbb.lib
│  │      tbb_debug.lib
│  │      WS2_32.Lib
│  │      zlib.lib
│  │      
│  ├─libxml2
│  │  ├─include
│  │  │  └─libxml
│  │  ...
│  ├─tbb
│  │  ├─include
│  │  │  ├─serial
│  │  │  │  └─tbb
│  │  │  └─tbb
│  │  │      ├─compat
│  │  │      ├─internal
│  │  │      └─machine
│  │  ├─lib
│  │  │   ├─ia32
│  │  │   │ 
│  │  │   └─intel64
│  │  │      ├─vc10
│  │  │      ├─vc12
│  │  ...     ...
│  └─zlib
│      ├─win32
│      ...
└─fractorium
    ├─archive
    ├─Builds
    │  ├─lib
    │  ├─QtCreator
    │  ...
    ├─Data
    │  
    ├─debian
    │  
    └─Source

```
##Begin to build
Open Qt Project `fractorium/main.pro` using Qt Creator with config like *Desktop Qt 5.5.1 MSVC2013 OpenGL 64bit*  
**DO TURN OFF** "shadow build option" in "Edit build configuration" for both `Debug` and `Release`

Switch to `Release` configuration and Build!  

You can find outputs `under fractorium\Bin\release` several minutes later if no error occurs.
```
Ember.dll
Ember.exp
Ember.lib
emberanimate.exe
EmberCL.dll
EmberCL.exp
EmberCL.lib
embergenome.exe
emberrender.exe
fractorium.exe
```

To run it, put exes and dlls above and these files together in one folder
```
Qt5.5.1\5.5\msvc2013_64\bin\Qt5OpenGL.dll
Qt5.5.1\5.5\msvc2013_64\bin\Qt5Widgets.dll
Qt5.5.1\5.5\msvc2013_64\bin\Qt5Core.dll
Qt5.5.1\5.5\msvc2013_64\bin\Qt5Gui.dll
Qt5.5.1\5.5\msvc2013_64\plugins\platforms\qwindows.dll  (put in folder "platforms")
External\libxml2\win32\bin.msvc\libxml2.dll
External\tbb\bin\intel64\vc12\tbb.dll
fractorium\Data\dark.qss
fractorium\Data\flam3-palettes.xml
```

To run on another computer, maybe these files are needed to be shipped with:
```
MSVC2013\VC\redist\x64\Microsoft.VC120.CRT\msvcp120.dll
MSVC2013\VC\redist\x64\Microsoft.VC120.CRT\msvcr120.dll
MSVC2013\VC\redist\x64\Microsoft.VC120.CRT\vccorlib120.dll
```
or you can install *Visual C++ Redistributable Packages for Visual Studio 2013 (64 bit)*

####Output file structure
```
[YOUR FOLDER]
│  dark.qss
│  Ember.dll
│  emberanimate.exe
│  EmberCL.dll
│  embergenome.exe
│  emberrender.exe
│  flam3-palettes.xml
│  fractorium.exe
│  libxml2.dll
│  Qt5Core.dll
│  Qt5Gui.dll
│  Qt5OpenGL.dll
│  Qt5Widgets.dll
│  tbb.dll
│ 
│  vccorlib120.dll   (optional)
│  msvcp120.dll      (optional)
│  msvcr120.dll      (optional)
│  
└─platforms
        qwindows.dll
```

####Have Fun!
