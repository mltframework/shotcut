[![build-snapflow-linux](https://github.com/mltframework/shotcut/workflows/build-snapflow-linux/badge.svg)](https://github.com/mltframework/shotcut/actions?query=workflow%3Abuild-snapflow-linux+is%3Acompleted+branch%3Amaster)
[![build-snapflow-macos](https://github.com/mltframework/shotcut/workflows/build-snapflow-macos/badge.svg)](https://github.com/mltframework/shotcut/actions?query=workflow%3Abuild-snapflow-macos+is%3Acompleted+branch%3Amaster)
[![build-snapflow-windows](https://github.com/mltframework/shotcut/workflows/build-snapflow-windows/badge.svg)](https://github.com/mltframework/shotcut/actions?query=workflow%3Abuild-snapflow-windows+is%3Acompleted+branch%3Amaster)

# Snapflow

**The editor got an upgrade — an assistant for your craft.**

A free, open source, cross-platform **video editor**, now with an inbuilt AI agent that
edits alongside you.

<div align="center">

<!-- TODO: replace with a real product screenshot/GIF -->
<img src="docs/img/preview.png" alt="Snapflow preview" width="800" />

</div>

## What's new: an agent that edits with you

Snapflow keeps the full manual editor and adds an inbuilt chat panel to prepare
timelines, edit media, and craft your video alongside an AI agent — you stay the
artist, the agent handles the busywork.

The agent can act on:

- **Frame** — inspect and adjust individual frames
- **Timeline** — arrange, trim, and reorder clips
- **Animations** — add and tune keyframed motion
- **Crop** — reframe shots
- **Timing** — adjust pacing, speed, and sync

### Bring your own model

Use your existing subscription — Snapflow talks to OpenAI (ChatGPT), Anthropic
(Claude), and other providers through a pluggable model layer, so you're not locked
into one vendor.

### MCP tool integrations

The agent connects to your editing and production tools over the
[Model Context Protocol](https://modelcontextprotocol.io/). Verified, widely-used MCP
servers you can wire in today:

| Server | Use in Snapflow |
|---|---|
| [Filesystem](https://github.com/modelcontextprotocol/servers/tree/main/src/filesystem) | Read/write project media and export files |
| [GitHub](https://github.com/modelcontextprotocol/servers/tree/main/src/github) | Version-control project files, script assets |
| [Fetch](https://github.com/modelcontextprotocol/servers/tree/main/src/fetch) | Pull reference media/pages from the web |
| [Memory](https://github.com/modelcontextprotocol/servers/tree/main/src/memory) | Persist project/style notes across sessions |
| [Puppeteer](https://github.com/modelcontextprotocol/servers/tree/main/src/puppeteer) | Capture web content as source footage |
| [Slack](https://github.com/modelcontextprotocol/servers) | Pull feedback/review threads into the edit |
| [Google Drive](https://github.com/modelcontextprotocol/servers) | Import/export project media |
| [Postgres](https://github.com/modelcontextprotocol/servers/tree/main/src/postgres) | Query production/asset metadata |
| [Sentry](https://github.com/modelcontextprotocol/servers) | Surface render/export errors |
| [Notion](https://github.com/makenotion/notion-mcp-server) | Pull shot lists / scripts into the timeline |

> This list is a starting point, not an endorsement of exclusivity — any spec-compliant
> MCP server works. Check each server's own repo for current install/auth steps before
> relying on it.

Use Snapflow itself as an MCP server so other agents/tools can drive the editor:

```json
{
  "mcpServers": {
    "snapflow": {
      "command": "snapflow",
      "args": ["--mcp-server"]
    }
  }
}
```

## Install

<!-- TODO: no hosted site/binaries yet — replace with real download links or a package
     manager entry once release infrastructure exists. Until then, build from source
     (see "How to build" below). -->

Build from source using the instructions below. Prebuilt binaries and an install
script are not available yet.

## Dependencies

Snapflow's direct (linked or hard runtime) dependencies are:

- [Shotcut](https://www.shotcut.org/): the video editor Snapflow is forked from
- [MLT](https://www.mltframework.org/): multimedia authoring framework
- [Qt 6 (6.4 minimum)](https://www.qt.io/): application and UI framework
- [FFTW](https://fftw.org/)
- [FFmpeg](https://www.ffmpeg.org/): multimedia format and codec libraries
- [Frei0r](https://www.dyne.org/software/frei0r/): video plugins
- [SDL](http://www.libsdl.org/): cross-platform audio playback

## License

GPLv3. See [COPYING](COPYING).

## Contributing

Contributions are welcome — see [CONTRIBUTING.md](CONTRIBUTING.md) for how to file
issues, propose changes, and the PR process.

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

If you want to translate Snapflow to another language, please use [Transifex](https://explore.transifex.com/ddennedy/shotcut/).
