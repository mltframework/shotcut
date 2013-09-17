# Shotcut - a free, open source, cross-platform video editor

Shotcut is a free, open source, cross-platform video editor. More informations and binaries can be found on the Shotcut website: http://www.shotcut.org.

# Features

Implemented features can be seen here: http://www.shotcut.org/bin/view/Shotcut/Features. Planned features can be found here: http://www.shotcut.org/bin/view/Shotcut/Roadmap.

# License

Shotcut is under GPLv3. See COPYING [here](COPYING).

# Contributors

[Dan Dennedy](http://www.dennedy.org) is the main author of Shotcut.

# How to build Shotcut

Building Shotcut should be reserved to developers, curious or packagers. If you only want to test latest Shotcut stable version, binaries are available on the website: http://www.shotcut.org.

## About Qt

`master` branch can be used to build Shotcut with Qt 4.8. However `qt5` branch (which should become default branch one time or another) can be used to build Shotcut against Qt 5.1 BUT NOT Qt 5.0 (under Linux and Mac only?) because Qt 5.0 miss x11extras module.

For example, Shotcut won't build with qt packages shipped in Ubuntu 13.04 or 13.10 because this is Qt 5.0. Instead you should install Qt dev files by yourself. You can use [online qt installer](http://qt-project.org/downloads), which is fast and can save you a long compilation time.

## Linux

First check [MLT framework](http://www.mltframework.org/) and [frei0r](http://www.piksel.org/frei0r) are correctly installed and available under `PKG_CONFIG_PATH` and `LD_LIBRARY_PATH`.

If dependencies are not in standard path, you should run these command before building Shotcut:

```
export PKG_CONFIG_PATH=PATH_TO_MLT_FRAMEWORK/lib/pkgconfig:$PKG_CONFIG_PATH
export LD_LIBRARY_PATH=PATH_TO_MLT_FRAMEWORK/lib/:$LD_LIBRARY_PATH
```

Now you can build Shotcut:

```
qmake && make
```

Currently Shotcut does not support well `make install`. So you'll have to execute it from source directory.

To execute Shotcut:

```
$ ./src/shotcut
```

## Mac OSX

Todo.

## Windows

Todo.
