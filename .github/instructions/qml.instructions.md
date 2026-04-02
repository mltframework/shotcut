---
applyTo: "src/qml/**"
---

# Shotcut QML Conventions

## Imports

- Use Qt 6 style — no version numbers: `import QtQuick` not `import QtQuick 2.15`.
- Standard filter UI imports:
  ```qml
  import QtQuick
  import QtQuick.Controls
  import QtQuick.Layouts
  import Shotcut.Controls as Shotcut
  ```
- Add `import org.shotcut.qml` (also aliased as `Shotcut`) when C++ QML types or enums are needed.
- `VuiBase.qml` only needs `import QtQuick`.

## Variable Declarations

- Use `let` and `const` in all new code — `const` for values never reassigned, `let` for values that are.
- Existing `var` code should not be changed unless you are already editing that block.
- Never use `var`, `let`, or `const` at component scope — use `property` declarations there.

## Component Structure

Declare in this order at the top of each component:
1. `id`
2. `property` declarations (public, then `_`-prefixed private)
3. `signal` declarations
4. `function` definitions
5. Child items and `Connections`

**Filter parameter UIs** (`ui.qml`): top-level `Item` or `Shotcut.KeyframableFilter`.  
**Filter VUIs** (`vui.qml`): top-level `Shotcut.VuiBase` (a `DropArea`).  
**Views**: top-level `Rectangle` or `Item`.  
**Meta files** (`meta.qml`): use `Metadata` from `org.shotcut.qml`; no copyright header.

## Properties

- `property var` — arrays and objects
- `property real` — fractional numbers
- `property int` — integers
- `property bool` — flags
- `property alias` — expose a child property
- `readonly property` — computed constants (use sparingly)
- `required property` — not used in this codebase
- Private properties use a `_` prefix: `_blockUpdate`, `_positionDragLocked`

## Functions

- All component-level functions are standalone `function foo() { }` — never arrow functions at the top level.
- Arrow functions are used only inside signal handlers and `Connections` method bodies.
- Standard filter helpers: `getPosition()`, `setControls()`, `updateFilter(position)`, `resetFilter()`.
- Standard VUI helpers: `setRectangleControl()`, `snapX(x)`, `snapY(y)`, `snapGrid(v, gridSize)`.

## Signal Handlers

- Inline handlers with parameters use Qt 6 arrow-function syntax:
  ```qml
  onPositionChanged: mouse => { ... }
  Keys.onPressed: event => { ... }
  ```
- Multi-parameter custom signal handlers:
  ```qml
  onTrimmingIn: (clip, delta, mouse) => { ... }
  ```

## Connections Blocks

- Use `function on<Signal>()` syntax (Qt 6 style) inside `Connections`.
- Place `target:` **last** in the block:
  ```qml
  Connections {
      function onChanged() { ... }
      function onPositionChanged() { ... }
      target: filter
  }
  ```

## Naming Conventions

- IDs, properties, functions, signals: `camelCase`
- Private properties and functions: `_camelCase` with leading underscore
- Top-level root items: `id: root` or `id: <role>Root`
- Handle sub-items: descriptive role names (`corner1Handle`, `positionHandle`)
- MLT parameter key strings: single-quoted (`'level'`, `'shotcut:rect'`)

## Code Conventions
- Follow the QML Coding Conventions in the Qt documentation: https://doc.qt.io/qt-6/qtqml-coding-conventions.html
- Use consistent indentation (4 spaces) and spacing around operators.
- Use `switch` statements for branching on discrete values (e.g., grid types), not if/else chains.

## Global Singletons

These are C++ context properties injected into the QML engine — never imported, just referenced directly:

| Name | Purpose |
|------|---------|
| `filter` | Current MLT filter — `filter.get()`, `filter.set()`, `filter.getRect()`, `filter.animateIn`, etc. |
| `producer` | Current MLT producer — `producer.position`, `producer.in` |
| `profile` | MLT profile — `profile.width`, `profile.height`, `profile.fps` |
| `video` | Video preview widget — `video.rect`, `video.zoom`, `video.grid`, `video.snapToGrid` |
| `application` | App helper — `application.showStatusMessage()`, `application.mousePos` |
| `settings` | `ShotcutSettings` — `settings.timelineRipple`, etc. |
| `multitrack` | Timeline model |
| `metadata` | Filter metadata for the current filter UI |

## Filter Parameter Read/Write

**Reading** (in `setControls()` / `setRectangleControl()`):
```qml
filter.getDouble('level', position)
filter.getRect('shotcut:rect', position)
filter.get('background')           // returns string
filter.animateIn / filter.animateOut / filter.duration
filter.isNew                       // check on Component.onCompleted
```

**Writing** (in `updateFilter(position)`):
```qml
filter.set('level', value, position)   // with keyframe position
filter.set('level', value)             // without keyframe
filter.resetProperty('level')         // clear all keyframes
```

**Standard keyframe update pattern** — guard with `_blockUpdate`, handle `animateIn`/`animateOut` ranges, call `filter.resetProperty` before setting boundary keyframes.

## Grid / Overlay Encoding

Integer `data()` on grid `QAction` (also exposed as `video.grid`):

| Range / Value | Meaning |
|---|---|
| `2`, `3`, `4`, `16` | NxN equal-division grid |
| `10010`, `10020` | Pixel grid — size = value − 10000 |
| `8090` | 80%/90% safe areas |
| `95` | EBU R95 safe areas |
| `20001`–`29999` | Aspect ratio frame overlay (`20001`=1:1, `20043`=4:3, `20169`=16:9, `20916`=9:16) |

Use a `switch` (not if/else) when branching on these values:
```qml
let ratioW, ratioH;
switch (video.grid) {
case 20001: ratioW = 1;  ratioH = 1;  break;
case 20169: ratioW = 16; ratioH = 9;  break;
case 20043: ratioW = 4;  ratioH = 3;  break;
default:    ratioW = 9;  ratioH = 16; break;
}
```

## Canvas Drawing (VuiBase)

- Two-pass drawing: black shadow (1px offset) then white line for contrast on any background.
- Get context with `const ctx = getContext("2d")` and call `ctx.reset()` first.
- Use `ctx` consistently — do not mix with `context`.
- Trigger repaints via `Connections` on `video` signals (`onRectChanged`, `onGridChanged`, etc.).

## Copyright Header

Add to every modified `.qml` file (except `meta.qml`), updating the year:
```qml
/*
 * Copyright (c) <year(s)> Meltytech, LLC
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
```
