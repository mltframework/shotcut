// SPDX-License-Identifier: GPL-2.0-or-later
import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    name: qsTr("360: Equirectangular to Stereographic")
    keywords: qsTr('spherical projection tiny small planet', 'search keywords for the 360: Equirectangular to Stereographic video filter') + ' 360: equirectangular stereographic bigsh0t #rgba'
    mlt_service: "frei0r.bigsh0t_eq_to_stereo"
    objectName: "bigsh0t_eq_to_stereo"
    qml: "ui.qml"
    vui: "vui.qml"
    icon: "icon.webp"

    keyframes {
        allowAnimateIn: true
        allowAnimateOut: true
        simpleProperties: ['yaw', "pitch", "roll", "fov", "amount"]
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
            },
            Parameter {
                name: qsTr('FOV', 'field of view')
                property: 'fov'
                isCurve: true
                minimum: 0
                maximum: 180
            },
            Parameter {
                name: qsTr('Amount')
                property: 'amount'
                isCurve: true
                minimum: 0
                maximum: 100
            }
        ]
    }
}
