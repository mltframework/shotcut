---
applyTo: "src/commands/**"
---

# Shotcut Commands Conventions

All files in `src/commands/` implement `QUndoCommand` subclasses that represent reversible operations. Each domain has a paired `.h`/`.cpp` and lives in a named namespace.

## Header Guards

Use `#ifndef` guards — no `#pragma once`:
```cpp
#ifndef FILTERCOMMANDS_H
#define FILTERCOMMANDS_H
// ...
#endif // FILTERCOMMANDS_H
```

## Namespaces

Each file defines a namespace matching its domain (`Timeline`, `Filter`, `Playlist`, `Markers`, `Subtitles`). No `using namespace` directives anywhere. Close with an annotated brace:
```cpp
} // namespace Timeline
```

## Command ID Enum

Each namespace declares an anonymous `enum` (not `enum class`) with `UndoId*` constants. Ranges are allocated per namespace to avoid collisions:

| Namespace | Range |
|---|---|
| `Playlist` | 0–99 |
| `Timeline` | 100–199 |
| `Markers` | 200–299 |
| `Filter` | 300–399 |
| `Subtitles` | 400–499 |

```cpp
enum {
    UndoIdTrimClipIn = 100,
    UndoIdTrimClipOut,
    UndoIdFadeIn,
    // ...
};
```

## Class Structure

```cpp
class SomeCommand : public QUndoCommand
{
public:
    SomeCommand(SomeModel &model, int index, ..., QUndoCommand *parent = 0);
    void redo();
    void undo();

protected:                              // only when mergeable
    int id() const { return UndoIdSomething; }
    bool mergeWith(const QUndoCommand *other);

private:
    SomeModel &m_model;
    int m_index;
    QString m_oldXml;
    QString m_newXml;
    // ...
};
```

- `redo()` and `undo()` are **not** marked `override` (codebase convention).
- `id()` is always inline in the header.
- `id()` and `mergeWith()` go in `protected:` and are omitted entirely when not needed.
- Default `parent` argument is `0`, not `nullptr`.

## Constructor Pattern

Always use a member initializer list with `QUndoCommand(parent)` first, then members in declaration order. Call `setText()` with `QObject::tr()` in the constructor body — not `tr()` directly (commands don't inherit `QObject`):

```cpp
AppendCommand::AppendCommand(MultitrackModel &model, int trackIndex,
                             const QString &xml, QUndoCommand *parent)
    : QUndoCommand(parent)
    , m_model(model)
    , m_trackIndex(qBound(0, trackIndex, qMax(model.rowCount() - 1, 0)))
    , m_xml(xml)
{
    setText(QObject::tr("Append to track"));
}
```

Bounds-checking (`qBound`, `qMax`) may be done inline in the initializer list.

## Member Variables

- All member variables use `m_` prefix.
- Model stored as reference: `SomeModel &m_model`.
- MLT producer stored by value: `Mlt::Producer m_producer`.
- UUID for deferred producer lookup: `QUuid m_producerUuid`.
- Before/after state: paired names `m_oldXxx` / `m_newXxx`.
- `UndoHelper m_undoHelper` for timeline commands needing before/after snapshots.

## MLT Object Lifecycle in redo()/undo()

Capture the producer by value in the constructor. On the first `redo()` call use the captured producer, then release it to save memory. On subsequent calls (and always in `undo()`), re-look up by UUID:

```cpp
void SomeCommand::redo()
{
    LOG_DEBUG() << "trackIndex" << m_trackIndex;
    Mlt::Producer producer = m_producer;
    if (!producer.is_valid())
        producer = findProducer(m_producerUuid);
    Q_ASSERT(producer.is_valid());
    // ... do work ...
    m_producer = Mlt::Producer();   // release after first redo
}

void SomeCommand::undo()
{
    LOG_DEBUG() << "trackIndex" << m_trackIndex;
    Mlt::Producer producer(findProducer(m_producerUuid));
    Q_ASSERT(producer.is_valid());
    // ... undo work ...
}
```

Construct a producer from XML with:
```cpp
Mlt::Producer producer(MLT.profile(), "xml-string", m_xml.toUtf8().constData());
```

## Logging

Call `LOG_DEBUG()` at the start of every `redo()` and `undo()` with enough context to identify the operation. Use `LOG_ERROR()` for unexpected states:
```cpp
void AppendCommand::redo()
{
    LOG_DEBUG() << "trackIndex" << m_trackIndex;
    ...
}
```

## Assertions

Use `Q_ASSERT(producer.is_valid())` after UUID-based lookups to catch programming errors in debug builds.

## Signals and Side Effects

Commands do not inherit `QObject` and cannot emit signals directly. Side effects go through:
- Model `do*` methods (which emit their own signals internally).
- `MAIN.*` calls for UI navigation (e.g., `MAIN.seekPlaylist(0)`).
- `emit MAIN.someSignal(...)` — rare, only when no model method covers it.
- `MAIN.filterController()->pauseUndoTracking()` / `resumeUndoTracking()` guards around bulk changes.

## mergeWith() Pattern

```cpp
bool TrimClipInCommand::mergeWith(const QUndoCommand *other)
{
    const TrimClipInCommand *that = static_cast<const TrimClipInCommand *>(other);
    if (that->id() != id() || that->m_trackIndex != m_trackIndex
            || that->m_clipIndex != m_clipIndex)
        return false;
    m_delta += that->m_delta;           // accumulate scalar changes
    // or: m_newValue = that->m_newValue;  // absorb latest "new" state
    return true;
}
```

- Use `static_cast`, not `dynamic_cast`, unless subclass-specific fields must be checked.
- Guard: return `false` if identity fields differ before merging.

## Include Order

In `.h` files: Qt/MLT/std headers only (what the header directly uses).  
In `.cpp` files: own header first, then project headers alphabetically, then Qt/system headers:
```cpp
#include "timelinecommands.h"

#include "Logger.h"
#include "mainwindow.h"
#include "mltcontroller.h"
#include "shotcut_mlt_properties.h"

#include <QMetaObject>
```

MLT C++ headers use angle brackets: `#include <MltProducer.h>`.

## Copyright Header

```cpp
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
