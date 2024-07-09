/*
 * Copyright (c) 2024 Meltytech, LLC
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
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import Shotcut.Controls as Shotcut
import org.shotcut.qml as Shotcut

Shotcut.KeyframableFilter {
    function setControls() {
        var feedIndex = feedCombo.find(filter.get("feed"));
        feedCombo.currentIndex = feedIndex;
        textFilterUi.setControls();
    }

    keyframableParameters: ['fgcolour', 'olcolour', 'bgcolour', 'opacity']
    startValues: [Qt.rgba(1, 1, 1, 1), Qt.rgba(0, 0, 0, 2.0 / 3.0), Qt.rgba(0, 0, 0, 0), 0.0]
    middleValues: [Qt.rgba(1, 1, 1, 1), Qt.rgba(0, 0, 0, 2.0 / 3.0), Qt.rgba(0, 0, 0, 0), 1.0]
    endValues: [Qt.rgba(1, 1, 1, 1), Qt.rgba(0, 0, 0, 2.0 / 3.0), Qt.rgba(0, 0, 0, 0), 0.0]
    width: 425
    height: 330
    Component.onCompleted: {
        filter.blockSignals = true;
        filter.set(textFilterUi.middleValue, Qt.rect(0, 0, profile.width, profile.height));
        filter.set(textFilterUi.startValue, Qt.rect(0, 0, profile.width, profile.height));
        filter.set(textFilterUi.endValue, Qt.rect(0, 0, profile.width, profile.height));
        if (filter.isNew) {
            if (application.OS === 'Windows')
                filter.set('family', 'Verdana');
            filter.set('fgcolour', '#ffffffff');
            filter.set('bgcolour', '#00000000');
            filter.set('olcolour', '#aa000000');
            filter.set('opacity', 1.0);
            filter.set('outline', 3);
            filter.set('weight', 700);
            filter.set('style', 'normal');
            filter.set(textFilterUi.useFontSizeProperty, false);
            filter.set('size', profile.height / 20);
            filter.set(textFilterUi.rectProperty, '20%/75%:60%x20%');
            filter.set(textFilterUi.valignProperty, 'bottom');
            filter.set(textFilterUi.halignProperty, 'center');
            filter.set('feed', feedCombo.textAt(0));
            filter.savePreset(preset.parameters);
        } else {
            if (filter.get('opacity') === null)
                filter.set('opacity', 1.0);
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

            parameters: textFilterUi.parameterList.concat(['feed'])
            onBeforePresetLoaded: {
                filter.resetProperty(textFilterUi.rectProperty);
                resetSimpleKeyframes();
            }
            onPresetSelected: {
                if (filter.get('opacity') === '')
                    filter.set('opacity', 1.0);
                setControls();
                textFilterUi.setKeyframedControls();
                initializeSimpleKeyframes();
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
            text: qsTr('Subtitle Track')
            Layout.alignment: Qt.AlignRight
        }

        Shotcut.ComboBox {
            id: feedCombo

            implicitWidth: 220
            textRole: 'text'
            onActivated: {
                if (feedCombo.currentIndex >= 0) {
                    filter.set("feed", feedCombo.currentText);
                }
            }

            model: subtitlesModel
        }

        Shotcut.TextFilterUi {
            id: textFilterUi
            showOpacity: true
            Layout.columnSpan: 2
        }

        Item {
            Layout.fillHeight: true
        }
    }
}
