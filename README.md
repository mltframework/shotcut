[![build-shotcut-linux](https://github.com/mltframework/shotcut/workflows/build-shotcut-linux/badge.svg)](https://github.com/mltframework/shotcut/actions?query=workflow%3Abuild-shotcut-linux+is%3Acompleted+branch%3Amaster)
[![build-shotcut-macos](https://github.com/mltframework/shotcut/workflows/build-shotcut-macos/badge.svg)](https://github.com/mltframework/shotcut/actions?query=workflow%3Abuild-shotcut-macos+is%3Acompleted+branch%3Amaster)
[![build-shotcut-windows](https://github.com/mltframework/shotcut/workflows/build-shotcut-windows/badge.svg)](https://github.com/mltframework/shotcut/actions?query=workflow%3Abuild-shotcut-windows+is%3Acompleted+branch%3Amaster)


# Shotcut - a free, open source, cross-platform **video editor**

<div align="center">

<img src="https://www.shotcut.org/assets/img/screenshots/Shotcut-18.11.18.png" alt="screenshot" />

</div>

- Features: https://www.shotcut.org/features/
- Roadmap: https://www.shotcut.org/roadmap/

## Install

Binaries are regularly built and are available at https://www.shotcut.org/download/.

## Contributors

- Dan Dennedy <<http://www.dennedy.org>> : main author
- Brian Matherly <<code@brianmatherly.com>> : contributor

## Dependencies

Shotcut's direct (linked or hard runtime) dependencies are:

- [MLT](https://www.mltframework.org/): multimedia authoring framework
- [Qt 6 (6.4 mininum)](https://www.qt.io/): application and UI framework
- [FFTW](https://fftw.org/)
- [FFmpeg](https://www.ffmpeg.org/): multimedia format and codec libraries
- [Frei0r](https://www.dyne.org/software/frei0r/): video plugins
- [SDL](http://www.libsdl.org/): cross-platform audio playback

See https://shotcut.org/credits/ for a more complete list including indirect
and bundled dependencies.

## License

GPLv3. See [COPYING](COPYING).

## How to build

**Warning**: building Shotcut should only be reserved to beta testers or contributors who know what they are doing.

### Qt Creator

The fastest way to build and try Shotcut development version is through [Qt Creator](https://www.qt.io/download#qt-creator).

### From command line

First, check dependencies are satisfied and various paths are correctly set to find different libraries and include files (Qt 5, MLT, frei0r and so forth).

#### Configure

In a new directory in which to make the build (separate from the source):

```
cmake -DCMAKE_INSTALL_PREFIX=/usr/local/ /path/to/shotcut
```

We recommend using the Ninja generator by adding `-GNinja` to the above command line.

#### Build

```
cmake --build .
```

#### Install

If you do not install, Shotcut may fail when you run it because it cannot locate its QML
files that it reads at run-time.

```
cmake --install .
```

## Translation

If you want to translate Shotcut to another language, please use [Transifex](https://www.transifex.com/ddennedy/shotcut/).
