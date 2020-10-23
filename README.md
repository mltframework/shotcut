[![build-shotcut-linux](https://github.com/mltframework/shotcut/workflows/build-shotcut-linux/badge.svg)](https://github.com/mltframework/shotcut/actions?query=workflow%3Abuild-shotcut-linux+is%3Acompleted+branch%3Amaster)
[![build-shotcut-windows](https://github.com/mltframework/shotcut/workflows/build-shotcut-windows/badge.svg)](https://github.com/mltframework/shotcut/actions?query=workflow%3Abuild-shotcut-windows+is%3Acompleted+branch%3Amaster)


# Shotcut - a free, open source, cross-platform **video editor**

- Features: https://www.shotcut.org/features/
- Roadmap: https://www.shotcut.org/roadmap/

## Install

Binaries are regularly built and are available at https://www.shotcut.org, on the Download page.

## Contributors

- Dan Dennedy <<http://www.dennedy.org>> : main author
- Brian Matherly <<code@brianmatherly.com>> : contributor

## Dependencies

Shotcut's direct (linked or hard runtime) dependencies are:

- [MLT](https://www.mltframework.org/): multimedia authoring framework
- [Qt 5](https://www.qt.io/): application and UI framework
- [FFmpeg](https://www.ffmpeg.org/): multimedia format and codec libraries
- [Frei0r](https://www.dyne.org/software/frei0r/): video plugins
- [SDL](http://www.libsdl.org/): cross-platform audio playback

See https://shotcut.org/credits/ for a more complete list including indirect
and bundled dependencies.

## Licence

GPLv3. See [COPYING](COPYING).

## How to build

**Warning**: building shotcut should only be reserved to beta testers or contributors who know what they are doing.

### Qt Creator

The fastest way to build and try Shotcut development version is probably using [Qt Creator](https://www.qt.io/download#qt-creator).

To make this easier, we provide [SDKs](https://shotcut.org/notes/) on the web site with each release that includes
Shotcut and all of its dependencies. These SDK pages also include setup instructions and tips on how to compile
MLT and other dependencies after updating.

### From command line

First, check dependencies are satisfied and various paths correctly set to find different libraries and include files (Qt 5, MLT, frei0r and so forth).

Build `Makefile`:

```
qmake PREFIX=/usr/local/
```
Compile `shotcut`:

```
make
```

`make install` is partially working.

Best way to test `shotcut` is to execute it from source folder:

```
./src/shotcut
```

## Translation

If you want to translate Shotcut to another language, please use [Transifex](https://www.transifex.com/ddennedy/shotcut/).
