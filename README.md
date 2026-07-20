[![build-snapflow-linux](https://github.com/mltframework/snapflow/workflows/build-snapflow-linux/badge.svg)](https://github.com/mltframework/snapflow/actions?query=workflow%3Abuild-snapflow-linux+is%3Acompleted+branch%3Amaster)
[![build-snapflow-macos](https://github.com/mltframework/snapflow/workflows/build-snapflow-macos/badge.svg)](https://github.com/mltframework/snapflow/actions?query=workflow%3Abuild-snapflow-macos+is%3Acompleted+branch%3Amaster)
[![build-snapflow-windows](https://github.com/mltframework/snapflow/workflows/build-snapflow-windows/badge.svg)](https://github.com/mltframework/snapflow/actions?query=workflow%3Abuild-snapflow-windows+is%3Acompleted+branch%3Amaster)


# Snapflow - a free, open source, cross-platform **video editor**

<div align="center">

<img src="https://www.snapflow.org/assets/img/screenshots/Snapflow-18.11.18.png" alt="screenshot" />

</div>

- Features: https://www.snapflow.org/features/
- Roadmap: https://www.snapflow.org/roadmap/

## Install

Binaries are regularly built and are available at https://www.snapflow.org/download/.

## Contributors

- Dan Dennedy <<http://www.dennedy.org>> : main author
- Brian Matherly <<code@brianmatherly.com>> : contributor

## Dependencies

Snapflow's direct (linked or hard runtime) dependencies are:

- [MLT](https://www.mltframework.org/): multimedia authoring framework
- [Qt 6 (6.4 minimum)](https://www.qt.io/): application and UI framework
- [FFTW](https://fftw.org/)
- [FFmpeg](https://www.ffmpeg.org/): multimedia format and codec libraries
- [Frei0r](https://www.dyne.org/software/frei0r/): video plugins
- [SDL](http://www.libsdl.org/): cross-platform audio playback

See https://snapflow.org/credits/ for a more complete list including indirect
and bundled dependencies.

## License

GPLv3. See [COPYING](COPYING).

## How to build

**Warning**: building Snapflow should only be reserved to beta testers or contributors who know what they are doing.

### Qt Creator

The fastest way to build and try Snapflow development version is through [Qt Creator](https://www.qt.io/download#qt-creator).

### From command line

First, check dependencies are satisfied and various paths are correctly set to find different libraries and include files (Qt, MLT, frei0r and so forth).

#### Configure

In a new directory in which to make the build (separate from the source):

```
cmake -DCMAKE_INSTALL_PREFIX=/usr/local/ /path/to/snapflow
```

We recommend using the Ninja generator by adding `-GNinja` to the above command line.

#### Build

```
cmake --build .
```

#### Install

If you do not install, Snapflow may fail when you run it because it cannot locate its QML
files that it reads at run-time.

```
cmake --install .
```

## Translation

If you want to translate Snapflow to another language, please use [Transifex](https://explore.transifex.com/ddennedy/snapflow/).
