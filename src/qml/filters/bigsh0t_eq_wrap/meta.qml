// SPDX-License-Identifier: GPL-2.0-or-later
import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    name: qsTr("360: Equirectangular Wrap")
    mlt_service: "frei0r.bigsh0t_eq_wrap"
    keywords: qsTr('spherical stretch', 'search keywords for the 360: Equirectangular Wrap video filter') + ' 360: equirectangular wrap bigsh0t'
    objectName: "bigsh0t_eq_wrap"
    qml: "ui.qml"
    keyframes {
        allowAnimateIn: true
        allowAnimateOut: true
        simpleProperties: ["hfov0", "hfov1", "vfov0", "vfov1", "blurStart", "blurEnd"]
        parameters: [
            Parameter {
                name: qsTr('hfov0')
                property: 'hfov0'
                isCurve: true
                minimum: -180
                maximum: 180
            },
            Parameter {
                name: qsTr('hfov1')
                property: 'hfov1'
                isCurve: true
                minimum: -180
                maximum: 180
            },
            Parameter {
                name: qsTr('vfov0')
                property: 'vfov0'
                isCurve: true
                minimum: -90
                maximum: 90
            },
            Parameter {
                name: qsTr('vfov1')
                property: 'vfov1'
                isCurve: true
                minimum: -90
                maximum: 90
            },
            Parameter {
                name: qsTr('blurStart')
                property: 'blurStart'
                isCurve: true
                minimum: 0.0
                maximum: 2.0
            },
            Parameter {
                name: qsTr('blurEnd')
                property: 'blurEnd'
                isCurve: true
                minimum: 0.0
                maximum: 2.0
            }
        ]
    }
}
