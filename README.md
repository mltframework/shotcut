# Shotcut - a free, open source, cross-platform **video editor**

- Features: http://www.shotcut.org/bin/view/Shotcut/Features
- Roadmap: http://www.shotcut.org/bin/view/Shotcut/Roadmap

## Install

**Warning**: Shotcut is currently in very early stage and active development. So do not expect a final product for now.

Binaries are regularly built and are available at http://www.shotcut.org, on the Download section.

## Contributors

- Dan Dennedy <<http://www.dennedy.org>> : main author
- Brian Matherly <<pez4brian@yahoo.com>> : contributor

## Dependencies

- [MLT](http://www.mltframework.org/): multimedia authoring framework
- [Qt 5](http://qt-project.org/): application and UI framework
- [FFmpeg](http://www.ffmpeg.org/): multimedia format and codec libraries
- [x264](http://www.videolan.org/developers/x264.html): H.264 encoder
- [WebM](http://www.webmproject.org/): VP8 encoder
- [LAME](http://lame.sourceforge.net/): MP3 encoder
- [Frei0r](http://www.dyne.org/software/frei0r/): video plugins
- [LADSPA](http://www.ladspa.org/): audio plugins

## Licence

GPLv3. See [COPYING](COPYING).

## How to build

**Warning**: building shotcut should only be reserved to beta testers or contributors who know what they are doing.

### Qt Creator

The fastest way to build and try Shotcut dev version is probably using [Qt Creator](http://qt-project.org/downloads#qt-creator).

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

Best way to tesst `shotcut` is to execute it from source folder:

```
./src/shotcut
```
