---
agent: agent
description: Scaffold all QML files for a new MLT filter UI in Shotcut.
---

Add a new MLT filter UI to Shotcut for the filter named **${input:filterName}** (display name) with MLT service identifier **${input:mltService}**.

## What to create

Create the directory `src/qml/filters/${input:mltService}/` and populate it with the files below.

### Always required

**`meta.qml`** — filter metadata, discovered automatically at runtime:
```qml
import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    name: qsTr('${input:filterName}')
    mlt_service: '${input:mltService}'
    keywords: qsTr('', 'search keywords for filter') + ' #${input:mltService}'
    qml: 'ui.qml'
    icon: 'icon.webp'

    keyframes {
        allowAnimateIn: true
        allowAnimateOut: true
        simpleProperties: ['level']   // list every keyframable parameter here
        parameters: [
            Parameter {
                name: qsTr('Level')
                property: 'level'
                // isCurve: true — only for numeric parameters with a well-defined range
                isCurve: true
                minimum: 0
                maximum: 1
            }
            // For string, color, or unbounded numeric parameters, omit isCurve
            // (or set isCurve: false).
        ]
    }
}
```
`simpleProperties` must list **every** keyframable parameter so the Keyframes panel shows them all. `isCurve: true` is appropriate only for numeric parameters with a defined `minimum`/`maximum`; omit it (or use `isCurve: false`) for color, enum/string, or effectively unbounded parameters.

**`ui.qml`** — filter parameter controls:

Use `Shotcut.KeyframableFilter` as the root whenever any parameter is keyframable. It provides `blockUpdate`, `getPosition()`, `isSimpleKeyframesActive()`, `updateFilter(parameter, value, button, position)`, `toggleKeyframes(checked, parameter, value)`, and manages `startValues`/`middleValues`/`endValues` automatically. List every keyframable parameter in `keyframableParameters` in the same order as `startValues`/`middleValues`/`endValues`.

```qml
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Shotcut.Controls as Shotcut

Shotcut.KeyframableFilter {
    property string levelParam: 'level'
    property double levelDefault: 1.0

    function setControls() {
        const position = getPosition();
        blockUpdate = true;
        slider.value = filter.getDouble(levelParam, position) * slider.maximumValue;
        levelKeyframesButton.checked = filter.animateIn <= 0 && filter.animateOut <= 0
                                       && filter.keyframeCount(levelParam) > 0;
        blockUpdate = false;
        enableControls(isSimpleKeyframesActive());
    }

    function enableControls(enabled) {
        slider.enabled = enabled;
    }

    function updateSimpleKeyframes() {
        updateFilter(levelParam, slider.value / slider.maximumValue, levelKeyframesButton, null);
    }

    keyframableParameters: [levelParam]
    startValues: [0.5]
    middleValues: [levelDefault]
    endValues: [0.5]
    width: 350
    height: 100

    Component.onCompleted: {
        if (filter.isNew) {
            filter.set(levelParam, levelDefault);
            filter.savePreset(preset.parameters);
        }
        setControls();
    }

    GridLayout {
        anchors.fill: parent
        anchors.margins: 8
        columns: 4

        Label {
            text: qsTr('Level')
            Layout.alignment: Qt.AlignRight
        }
        Shotcut.SliderSpinner {
            id: slider
            minimumValue: 0
            maximumValue: 100
            decimals: 1
            suffix: ' %'
            onValueChanged: updateFilter(levelParam, value / maximumValue, levelKeyframesButton, getPosition())
        }
        Shotcut.UndoButton { onClicked: slider.value = levelDefault * slider.maximumValue }
        Shotcut.KeyframesButton {
            id: levelKeyframesButton
            onToggled: {
                enableControls(true);
                toggleKeyframes(checked, levelParam, slider.value / slider.maximumValue);
            }
        }

        Item { Layout.fillHeight: true }
        Shotcut.Preset {
            id: preset
            parameters: [levelParam]
            onPresetSelected: setControls()
        }
    }

    Connections {
        function onChanged() { setControls(); }
        function onInChanged() { updateSimpleKeyframes(); }
        function onOutChanged() { updateSimpleKeyframes(); }
        function onAnimateInChanged() { updateSimpleKeyframes(); }
        function onAnimateOutChanged() { updateSimpleKeyframes(); }
        function onPropertyChanged(name) { setControls(); }
        target: filter
    }

    Connections {
        function onPositionChanged() { setControls(); }
        target: producer
    }
}
```

Use a plain `Item` root only for filters with **no** keyframable parameters (e.g., a simple combo-box-only filter). In that case omit all keyframe machinery and manage `blockUpdate` manually as a `property bool`.

### Optional: VUI overlay (`vui.qml`)

Only add a VUI if the filter needs interactive handles drawn on the canvas (e.g., rectangle, corners, position). Declare it in `meta.qml` with `vui: 'vui.qml'`.

For a rectangle-based VUI, delegate to the shared component:
```qml
import Shotcut.Controls as Shotcut

Shotcut.TextFilterVui {
    rectProperty: 'rect'
}
```

For a custom VUI with interactive handles, use `Shotcut.VuiBase` as the root and place a `Shotcut.RectangleControl` (or custom handles) inside a `Flickable`. See `src/qml/filters/mask_shape/vui.qml` or `src/qml/filters/corners/vui.qml` for reference.

Also declare the rectangle parameter in `meta.qml`:
```qml
Parameter {
    name: qsTr('Position / Size')
    property: 'rect'
    isRectangle: true
}
```

## After creating the files

Run the install step so Shotcut can find the new folder at runtime:
```bash
cmake --install build/cc-debug
```

No `CMakeLists.txt`, `resources.qrc`, or `qmldir` changes are needed — the entire `qml/` tree is installed automatically and `FilterController` scans all `meta*.qml` files at startup. The filter only appears in the UI if its `mlt_service` plugin is present in the MLT repository.

## Conventions

- All new JS inside QML files must use `let`/`const`, not `var`.
- No import version numbers: `import QtQuick` not `import QtQuick 2.15`.
- Connections blocks: `function onSignal()` style with `target:` placed last.
- Add the standard GPL copyright header to every `.qml` file (see `qml.instructions.md`).
- `meta.qml` files do **not** get a copyright header.
