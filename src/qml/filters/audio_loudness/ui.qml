/*
 * Copyright (c) 2013-2015 Meltytech, LLC
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
import QtQuick.Dialogs 1.1
import QtQuick.Controls 1.0
import QtQuick.Layouts 1.0
import Shotcut.Controls 1.0

Item {
    width: 350
    height: 50
    
    function setStatus( inProgress ) {
        if (inProgress) {
            status.text = qsTr('Analyzing...')
        }
        else if (filter.get("results").length > 0 ) {
            status.text = qsTr('Analysis complete.')
        }
        else
        {
            status.text = qsTr('Click "Analyze" to use this filter.')
        }
    }

    Connections {
        target: filter
        onAnalyzeFinished: {
            setStatus(false)
            button.enabled = true
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 8

        RowLayout {
            Label { text: qsTr('Target Loudness') }
            SliderSpinner {
                id: programSlider
                minimumValue: -50.0
                maximumValue: -10.0
                decimals: 1
                suffix: ' LUFS'
                spinnerWidth: 100
                value: filter.getDouble('program')
                onValueChanged: filter.set('program', value)
            }
            UndoButton {
                onClicked: programSlider.value = -23.0
            }
        }

        RowLayout {
            Button {
                id: button
                text: qsTr('Analyze')
                onClicked: {
                    button.enabled = false
                    filter.analyze(true);
                }
            }
            Label {
                id: status
                Component.onCompleted: {
                    setStatus(false)
                }
            }
        }

        Item {
            Layout.fillHeight: true;
        }
    }
}
