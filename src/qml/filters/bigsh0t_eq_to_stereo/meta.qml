/* SPDX-License-Identifier: GPL-2.0-or-later */
import QtQuick 2.0
import org.shotcut.qml 1.0

Metadata {
    type: Metadata.Filter
    name: qsTr("360: Equirectangular to Stereographic")
    mlt_service: "frei0r.bigsh0t_eq_to_stereo"
    objectName: "bigsh0t_eq_to_stereo"
    qml: "ui.qml"
    vui: "vui.qml"
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
                name: qsTr('Pitch')
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
                name: qsTr('FOV')
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
