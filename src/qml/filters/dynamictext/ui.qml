/*
 * Copyright (c) 2014-2021 Meltytech, LLC
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

import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import Shotcut.Controls 1.0 as Shotcut

Item {
    function setControls() {
        textArea.text = filter.get('argument');
        textFilterUi.setControls();
    }

    width: 500
    height: 380
    Component.onCompleted: {
        filter.blockSignals = true;
        filter.set(textFilterUi.middleValue, Qt.rect(0, 0, profile.width, profile.height));
        filter.set(textFilterUi.startValue, Qt.rect(0, 0, profile.width, profile.height));
        filter.set(textFilterUi.endValue, Qt.rect(0, 0, profile.width, profile.height));
        if (filter.isNew) {
            var presetParams = preset.parameters.slice();
            var index = presetParams.indexOf('argument');
            if (index > -1)
                presetParams.splice(index, 1);

            if (application.OS === 'Windows')
                filter.set('family', 'Verdana');

            filter.set('fgcolour', '#ffffffff');
            filter.set('bgcolour', '#00000000');
            filter.set('olcolour', '#aa000000');
            filter.set('outline', 3);
            filter.set('weight', 10 * Font.Normal);
            filter.set('style', 'normal');
            filter.set(textFilterUi.useFontSizeProperty, false);
            filter.set('size', profile.height);
            filter.set(textFilterUi.rectProperty, '0%/50%:50%x50%');
            filter.set(textFilterUi.valignProperty, 'bottom');
            filter.set(textFilterUi.halignProperty, 'left');
            filter.savePreset(presetParams, qsTr('Bottom Left'));
            filter.set(textFilterUi.rectProperty, '50%/50%:50%x50%');
            filter.set(textFilterUi.valignProperty, 'bottom');
            filter.set(textFilterUi.halignProperty, 'right');
            filter.savePreset(presetParams, qsTr('Bottom Right'));
            filter.set(textFilterUi.rectProperty, '0%/0%:50%x50%');
            filter.set(textFilterUi.valignProperty, 'top');
            filter.set(textFilterUi.halignProperty, 'left');
            filter.savePreset(presetParams, qsTr('Top Left'));
            filter.set(textFilterUi.rectProperty, '50%/0%:50%x50%');
            filter.set(textFilterUi.valignProperty, 'top');
            filter.set(textFilterUi.halignProperty, 'right');
            filter.savePreset(presetParams, qsTr('Top Right'));
            filter.set(textFilterUi.rectProperty, '0%/76%:100%x14%');
            filter.set(textFilterUi.valignProperty, 'bottom');
            filter.set(textFilterUi.halignProperty, 'center');
            filter.savePreset(presetParams, qsTr('Lower Third'));
            // Add some animated presets.
            filter.animateIn = Math.round(profile.fps);
            filter.set(textFilterUi.rectProperty, '0=-100%/0%:100%x100%; :1.0=0%/0%:100%x100%');
            filter.savePreset(presetParams.concat('shotcut:animIn'), qsTr('Slide In From Left'));
            filter.set(textFilterUi.rectProperty, '0=100%/0%:100%x100%; :1.0=0%/0%:100%x100%');
            filter.savePreset(presetParams.concat('shotcut:animIn'), qsTr('Slide In From Right'));
            filter.set(textFilterUi.rectProperty, '0=0%/-100%:100%x100%; :1.0=0%/0%:100%x100%');
            filter.savePreset(presetParams.concat('shotcut:animIn'), qsTr('Slide In From Top'));
            filter.set(textFilterUi.rectProperty, '0=0%/100%:100%x100%; :1.0=0%/0%:100%x100%');
            filter.savePreset(presetParams.concat('shotcut:animIn'), qsTr('Slide In From Bottom'));
            filter.animateIn = 0;
            filter.animateOut = Math.round(profile.fps);
            filter.set(textFilterUi.rectProperty, ':-1.0=0%/0%:100%x100%; -1=-100%/0%:100%x100%');
            filter.savePreset(presetParams.concat('shotcut:animOut'), qsTr('Slide Out Left'));
            filter.set(textFilterUi.rectProperty, ':-1.0=0%/0%:100%x100%; -1=100%/0%:100%x100%');
            filter.savePreset(presetParams.concat('shotcut:animOut'), qsTr('Slide Out Right'));
            filter.set(textFilterUi.rectProperty, ':-1.0=0%/0%:100%x100%; -1=0%/-100%:100%x100%');
            filter.savePreset(presetParams.concat('shotcut:animOut'), qsTr('Slide Out Top'));
            filter.set(textFilterUi.rectProperty, ':-1.0=0%/0%:100%x100%; -1=0%/100%:100%x100%');
            filter.savePreset(presetParams.concat('shotcut:animOut'), qsTr('Slide Out Bottom'));
            filter.animateOut = 0;
            filter.animateIn = filter.duration;
            filter.set(textFilterUi.rectProperty, '0=0%/0%:100%x100%; -1=-5%/-5%:110%x110%');
            filter.savePreset(presetParams.concat('shotcut:animIn'), qsTr('Slow Zoom In'));
            filter.set(textFilterUi.rectProperty, '0=-5%/-5%:110%x110%; -1=0%/0%:100%x100%');
            filter.savePreset(presetParams.concat('shotcut:animIn'), qsTr('Slow Zoom Out'));
            filter.set(textFilterUi.rectProperty, '0=-5%/-5%:110%x110%; -1=-10%/-5%:110%x110%');
            filter.deletePreset(qsTr('Slow Pan Left'));
            filter.savePreset(presetParams.concat('shotcut:animIn'), qsTr('Slow Move Left'));
            filter.set(textFilterUi.rectProperty, '0=-5%/-5%:110%x110%; -1=0%/-5%:110%x110%');
            filter.deletePreset(qsTr('Slow Pan Right'));
            filter.savePreset(presetParams.concat('shotcut:animIn'), qsTr('Slow Move Right'));
            filter.set(textFilterUi.rectProperty, '0=-5%/-5%:110%x110%; -1=-5%/-10%:110%x110%');
            filter.deletePreset(qsTr('Slow Pan Up'));
            filter.savePreset(presetParams.concat('shotcut:animIn'), qsTr('Slow Move Up'));
            filter.set(textFilterUi.rectProperty, '0=-5%/-5%:110%x110%; -1=-5%/0%:110%x110%');
            filter.deletePreset(qsTr('Slow Pan Down'));
            filter.savePreset(presetParams.concat('shotcut:animIn'), qsTr('Slow Move Down'));
            filter.set(textFilterUi.rectProperty, '0=0%/0%:100%x100%; -1=-10%/-10%:110%x110%');
            filter.deletePreset(qsTr('Slow Zoom In, Pan Up Left'));
            filter.savePreset(presetParams.concat('shotcut:animIn'), qsTr('Slow Zoom In, Move Up Left'));
            filter.set(textFilterUi.rectProperty, '0=0%/0%:100%x100%; -1=0%/0%:110%x110%');
            filter.deletePreset(qsTr('Slow Zoom In, Pan Down Right'));
            filter.savePreset(presetParams.concat('shotcut:animIn'), qsTr('Slow Zoom In, Move Down Right'));
            filter.set(textFilterUi.rectProperty, '0=-10%/0%:110%x110%; -1=0%/0%:100%x100%');
            filter.deletePreset(qsTr('Slow Zoom Out, Pan Up Right'));
            filter.savePreset(presetParams.concat('shotcut:animIn'), qsTr('Slow Zoom Out, Move Up Right'));
            filter.set(textFilterUi.rectProperty, '0=0%/-10%:110%x110%; -1=0%/0%:100%x100%');
            filter.deletePreset(qsTr('Slow Zoom Out, Pan Down Left'));
            filter.savePreset(presetParams.concat('shotcut:animIn'), qsTr('Slow Zoom Out, Move Down Left'));
            filter.animateIn = 0;
            filter.resetProperty(textFilterUi.rectProperty);
            // Add default preset.
            filter.set(textFilterUi.rectProperty, '0%/0%:100%x100%');
            filter.savePreset(presetParams);
        } else {
            filter.set(textFilterUi.middleValue, filter.getRect(textFilterUi.rectProperty, filter.animateIn + 1));
            if (filter.animateIn > 0)
                filter.set(textFilterUi.startValue, filter.getRect(textFilterUi.rectProperty, 0));

            if (filter.animateOut > 0)
                filter.set(textFilterUi.endValue, filter.getRect(textFilterUi.rectProperty, filter.duration - 1));

        }
        filter.blockSignals = false;
        setControls();
        if (filter.isNew)
            filter.set(textFilterUi.rectProperty, filter.getRect(textFilterUi.rectProperty));

    }

    GridLayout {
        id: textGrid

        columns: 2
        anchors.fill: parent
        anchors.margins: 8

        Label {
            text: qsTr('Preset')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.Preset {
            id: preset

            parameters: textFilterUi.parameterList.concat(['argument'])
            onBeforePresetLoaded: {
                filter.resetProperty(textFilterUi.rectProperty);
            }
            onPresetSelected: {
                setControls();
                filter.blockSignals = true;
                filter.set(textFilterUi.middleValue, filter.getRect(textFilterUi.rectProperty, filter.animateIn + 1));
                if (filter.animateIn > 0)
                    filter.set(textFilterUi.startValue, filter.getRect(textFilterUi.rectProperty, 0));

                if (filter.animateOut > 0)
                    filter.set(textFilterUi.endValue, filter.getRect(textFilterUi.rectProperty, filter.duration - 1));

                filter.blockSignals = false;
            }
        }

        Label {
            text: qsTr('Text')
            Layout.alignment: Qt.AlignRight | Qt.AlignTop
        }

        Item {
            Layout.minimumHeight: fontMetrics.height * 6
            Layout.maximumHeight: Layout.minimumHeight
            Layout.minimumWidth: preset.width
            Layout.maximumWidth: preset.width

            FontMetrics {
                id: fontMetrics

                font: textArea.font
            }

            ScrollView {
                id: scrollview

                width: preset.width - (ScrollBar.vertical.visible ? 16 : 0)
                height: parent.height - (ScrollBar.horizontal.visible ? 16 : 0)
                clip: true

                TextArea {
                    id: textArea

                    property int maxLength: 256

                    textFormat: TextEdit.PlainText
                    wrapMode: TextEdit.NoWrap
                    selectByMouse: true
                    persistentSelection: true
                    padding: 0
                    text: '__empty__'
                    onTextChanged: {
                        if (text === '__empty__')
                            return ;

                        if (length > maxLength) {
                            text = text.substring(0, maxLength);
                            cursorPosition = maxLength;
                        }
                        if (!parseInt(filter.get(textFilterUi.useFontSizeProperty)))
                            filter.set('size', profile.height / text.split('\n').length);

                        filter.set('argument', text);
                    }
                    Keys.onPressed: {
                        if (event.key === Qt.Key_V && (event.modifiers & Qt.ShiftModifier) && (event.modifiers & Qt.ControlModifier || event.modifiers & Qt.MetaModifier)) {
                            event.accepted = true;
                            textArea.paste();
                        }
                    }

                    MouseArea {
                        acceptedButtons: Qt.RightButton
                        anchors.fill: parent
                        onClicked: contextMenu.popup()
                    }

                    Shotcut.EditMenu {
                        id: contextMenu
                    }

                    background: Rectangle {
                        anchors.fill: parent
                        color: textArea.palette.base
                    }
                    // workaround initialization problem

                }

                ScrollBar.horizontal: ScrollBar {
                    height: 16
                    policy: ScrollBar.AlwaysOn
                    visible: scrollview.contentWidth > scrollview.width
                    parent: scrollview.parent
                    anchors.top: scrollview.bottom
                    anchors.left: scrollview.left
                    anchors.right: scrollview.right
                }

                ScrollBar.vertical: ScrollBar {
                    width: 16
                    policy: ScrollBar.AlwaysOn
                    visible: scrollview.contentHeight > scrollview.height
                    parent: scrollview.parent
                    anchors.top: scrollview.top
                    anchors.left: scrollview.right
                    anchors.bottom: scrollview.bottom
                }

            }

        }

        Label {
            text: qsTr('Insert field')
            Layout.alignment: Qt.AlignRight
        }

        RowLayout {
            Shotcut.Button {
                text: qsTr('# (Hash sign)')
                onClicked: textArea.insert(textArea.cursorPosition, '\\#')
            }

            Shotcut.Button {
                text: qsTr('Timecode')
                onClicked: textArea.insert(textArea.cursorPosition, '#timecode#')
            }

            Shotcut.Button {
                text: qsTr('Frame #', 'Frame number')
                onClicked: textArea.insert(textArea.cursorPosition, '#frame#')
            }

            Shotcut.Button {
                text: qsTr('File date')
                onClicked: textArea.insert(textArea.cursorPosition, '#localfiledate#')
            }

            Shotcut.Button {
                text: qsTr('File name')
                onClicked: textArea.insert(textArea.cursorPosition, '#resource#')
            }

        }

        Shotcut.TextFilterUi {
            id: textFilterUi

            Layout.columnSpan: 2
        }

        Item {
            Layout.fillHeight: true
        }

    }

}
