# Shotcut — Workspace Instructions

Shotcut is a free, open-source, cross-platform video editor built on Qt 6 and the [MLT multimedia framework](https://www.mltframework.org/).

## Build and Run

```bash
# Configure (from repo root; output goes to build/cc-debug)
cmake --preset cc-debug

# Reformat code with clang-format
cmake --build build/cc-debug --target clang-format

# Build
cmake --build build/cc-debug

# Run directly from build tree
build/cc-debug/src/shotcut
```

The default preset `cc-debug` (defined in `CMakePresets.json`) uses Ninja, gcc or clang from `$PATH` and Qt 6.8.3 from `~/Qt/6.8.3`, and produces a Debug build.

To properly debug on Windows you must set the environment variable `QSG_RHI_BACKEND=d3d11` to prevent running as a child process detached from the debugger.

## Architecture

| Singleton | Macro | Role |
|-----------|-------|------|
| `Mlt::Controller` | `MLT` | Media engine: producer/consumer/seek/profile |
| `MainWindow` | `MAIN` | Central hub; owns all docks, undo stack, layout |
| `ShotcutActions` | `Actions` | Global `QAction` registry keyed by string |
| `ShotcutSettings` | `Settings` | `QSettings`-backed properties; bindable from QML |

**Data flow**: UI events → `Actions["someKey"]` → undoable `QUndoCommand` in `src/commands/` → `MLT` producer/filter mutation → Qt signals → model updates → QML views re-render.

**QML views** (in `src/qml/views/`): Timeline, Filter, and Keyframes panels are QML; individual filter parameter UIs live in `src/qml/filters/<filter-name>/`.

**Docks** (`src/docks/`): All panels are `QDockWidget` subclasses — Timeline, Playlist, Encode, Filters, Keyframes, Markers, Subtitles, Jobs, Recent, Files, Scope, Notes.

**Models** (`src/models/`): Qt item models bridge C++ data to QML. `MultitrackModel` is the timeline; `PlaylistModel` is the source bin.

## Code Conventions

- **Member variables**: `m_` prefix (e.g., `m_position`, `m_gridButton`)
- **Class naming**: `PascalCase`; methods/variables: `camelCase`
- **Slot naming**: `on<ObjectName><SignalName>()` (e.g., `onGridToggled`, `onFrameDisplayed`)
- **Header guards**: `#ifndef CLASSNAME_H` / `#define CLASSNAME_H`
- **Signal/slot style**: Old `SIGNAL()`/`SLOT()` macros are still common in this codebase — do not replace them with new-style pointer syntax unless already using new-style in the same block. Lambda connects are used for new code. However, do prefer the new style of `QObject::connect()` with function pointers or lambdas for new code.
- **Action registration**: Add new actions in `setupActions()` or analogous setup functions; register with `Actions.add("keyName", action, tr("Group"))`.
- **Grid/zoom data encoding**: Integer `data()` on `QAction` encodes the grid type — values ≥ 10000 mean pixel grids (e.g., `10010` = 10px, `10020` = 20px); values like `8090` and `95` encode safe-area presets; values 20001–29999 encode aspect ratio frame overlays (`20001`=1:1, `20043`=4:3, `20169`=16:9, `20916`=9:16).
- Update the copyright year in all modified files.

## Key Entry Points

- `src/main.cpp` — application entry
- `src/mainwindow.cpp` — central UI coordination
- `src/player.cpp` — video player widget with transport/zoom/grid controls
- `src/mltcontroller.cpp` — MLT wrapper
- `src/actions.cpp` — global action registry
- `src/settings.cpp` — persistent settings

## No Automated Tests

There are no C++ unit tests. The only automated tests are JavaScript tests for EDL/chapters export:

```bash
node src/qml/export-edl/test-node.js
node src/qml/export-chapters/test-node.js
```

## Dependencies

Qt 6.4+, MLT++ ≥ 7.36.0 (via pkg-config), FFTW, FFmpeg, Frei0r, SDL2. See `CMakeLists.txt` for the full list.
