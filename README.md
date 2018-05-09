# Shotcut - a free, open source, cross-platform **video editor**

- Features: https://www.shotcut.org/features/
- Roadmap: https://www.shotcut.org/roadmap/

## Install

**Warning**: Shotcut is currently in very early stage and active development. So do not expect a final product for now.

Binaries are regularly built and are available at https://www.shotcut.org, on the Download section.

## Contributors

- Dan Dennedy <<http://www.dennedy.org>> : main author
- Brian Matherly <<code@brianmatherly.com>> : contributor

## Dependencies

- [MLT](https://www.mltframework.org/): multimedia authoring framework
- [Qt 5](https://www.qt.io/): application and UI framework
- [FFmpeg](https://www.ffmpeg.org/): multimedia format and codec libraries
- [x264](https://www.videolan.org/developers/x264.html): H.264 encoder
- [WebM](https://www.webmproject.org/): VP8 encoder
- [LAME](http://lame.sourceforge.net/): MP3 encoder
- [Frei0r](https://www.dyne.org/software/frei0r/): video plugins
- [LADSPA](https://www.ladspa.org/): audio plugins
- [WebVfx](https://github.com/mltframework/webvfx): video effects using web technologies
- [Movit](https://git.sesse.net/?p=movit)

## Licence

GPLv3. See [COPYING](COPYING).

## How to build

**Warning**: building shotcut should only be reserved to beta testers or contributors who know what they are doing.

### Qt Creator

The fastest way to build and try Shotcut dev version is probably using [Qt Creator](https://www.qt.io/download#qt-creator).

### From command line

First, check dependencies are satisfied and various paths correctly set to find different libraries and include files (Qt 5, MLT, Frei0r and so forth).

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
