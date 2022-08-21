// SPDX-License-Identifier: GPL-2.0-or-later
import QtQuick 2.0
import org.shotcut.qml 1.0

Metadata {
    type: Metadata.Filter
    name: qsTr("360: Transform")
    mlt_service: "frei0r.bigsh0t_transform_360"
    keywords: qsTr('spherical yaw pitch roll', 'search keywords for the 360: Transform video filter') + ' 360: transform bigsh0t'
    objectName: "bigsh0t_transform_360"
    qml: "ui.qml"
    vui: "vui.qml"

    keyframes {
        allowAnimateIn: true
        allowAnimateOut: true
        simpleProperties: ['yaw', "pitch", "roll"]
        parameters: [
            Parameter {
                name: qsTr('Yaw')
                property: 'yaw'
                isCurve: true
                minimum: -360
                maximum: 360
            },
            Parameter {
                name: qsTr('Pitch', 'rotation around the side-to-side axis (roll, pitch, yaw)')
                property: 'pitch'
                isCurve: true
                minimum: -180
                maximum: 180
            },
            Parameter {
                name: qsTr('Roll')
                property: 'roll'
                isCurve: true
                minimum: -180
                maximum: 180
            }
        ]
    }

}
