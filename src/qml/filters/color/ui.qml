/*
 * Copyright (c) 2014 Meltytech, LLC
 * Author: Brian Matherly <pez4brian@yahoo.com>
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

import QtQuick 2.1
import QtQuick.Controls 1.1
import QtQuick.Layouts 1.0
import Shotcut.Controls 1.0

Rectangle {
    property var defaultParameters: ['lift_r', 'lift_g', 'lift_b', 'gamma_r', 'gamma_g', 'gamma_b', 'gain_r', 'gain_g', 'gain_b']
    property var gammaFactor: 2.0
    property var gainFactor: 4.0
    width: 400
    height: 200
    color: 'transparent'
    
    function loadWheels() {
        liftwheel.color = Qt.rgba( filter.get("lift_r"),
                                   filter.get("lift_g"),
                                   filter.get("lift_b"),
                                   1.0 )
        gammawheel.color = Qt.rgba( filter.get("gamma_r") / gammaFactor,
                                    filter.get("gamma_g") / gammaFactor,
                                    filter.get("gamma_b") / gammaFactor,
                                    1.0 )
        gainwheel.color = Qt.rgba( filter.get("gain_r") / gainFactor,
                                   filter.get("gain_g") / gainFactor,
                                   filter.get("gain_b") / gainFactor,
                                   1.0 )
    }
    
    Component.onCompleted: {
        if (filter.isNew) {
            // Set default parameter values
            filter.set("lift_r", 0.0);
            filter.set("lift_g", 0.0);
            filter.set("lift_b", 0.0);
            filter.set("gamma_r", 1.0);
            filter.set("gamma_g", 1.0);
            filter.set("gamma_b", 1.0);
            filter.set("gain_r", 1.0);
            filter.set("gain_g", 1.0);
            filter.set("gain_b", 1.0);
            filter.savePreset(defaultParameters)
        }
        loadWheels()
    }

    GridLayout {
        columns: 6
        columnSpacing: 20
        anchors.top: parent.top
        anchors.bottom: parent.bottom

        // Row 1
        Preset {
            Layout.columnSpan: 6
            parameters: defaultParameters
            onPresetSelected: {
                loadWheels()
            }
        }

        // Row 2
        Label { text: qsTr('Shadows (Lift)') }
        UndoButton {
            Layout.alignment: Qt.AlignRight
            onClicked: liftwheel.color = Qt.rgba( 0.0, 0.0, 0.0, 1.0 )
        }
        Label { text: qsTr('Midtones (Gamma)') }
        UndoButton {
            Layout.alignment: Qt.AlignRight
            onClicked: gammawheel.color = Qt.rgba( 1.0 / gammaFactor, 1.0 / gammaFactor, 1.0 / gammaFactor, 1.0 )
        }
        Label { text: qsTr('Highlights (Gain)') }
        UndoButton {
            Layout.alignment: Qt.AlignRight
            onClicked: gainwheel.color = Qt.rgba( 1.0 / gainFactor, 1.0 / gainFactor, 1.0 / gainFactor, 1.0 )
        }
        
        // Row 3
        ColorWheelItem {
            id: liftwheel
            implicitWidth: height * 1.1
            Layout.columnSpan: 2
            Layout.fillHeight: true;
            Layout.alignment : Qt.AlignCenter | Qt.AlignTop
            Layout.minimumHeight: 75
            Layout.maximumHeight: 300
            onColorChanged: {
                filter.set("lift_r", liftwheel.red / 255.0 );
                filter.set("lift_g", liftwheel.green / 255.0 );
                filter.set("lift_b", liftwheel.blue / 255.0 );
            }
        }
        ColorWheelItem {
            id: gammawheel
            implicitWidth: height * 1.1
            Layout.columnSpan: 2
            Layout.fillHeight: true;
            Layout.alignment : Qt.AlignCenter | Qt.AlignTop
            Layout.minimumHeight: 75
            Layout.maximumHeight: 300
            onColorChanged: {
                filter.set("gamma_r", (gammawheel.red / 255.0) * gammaFactor);
                filter.set("gamma_g", (gammawheel.green / 255.0) * gammaFactor);
                filter.set("gamma_b", (gammawheel.blue / 255.0) * gammaFactor);
            }
        }
        ColorWheelItem {
            id: gainwheel
            implicitWidth: height * 1.1
            Layout.columnSpan: 2
            Layout.fillHeight: true;
            Layout.alignment : Qt.AlignCenter | Qt.AlignTop
            Layout.minimumHeight: 75
            Layout.maximumHeight: 300
            onColorChanged: {
                filter.set("gain_r", (gainwheel.red / 255.0) * gainFactor);
                filter.set("gain_g", (gainwheel.green / 255.0) * gainFactor);
                filter.set("gain_b", (gainwheel.blue / 255.0) * gainFactor);
            }
        }
    }
}
