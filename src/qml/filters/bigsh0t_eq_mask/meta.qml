// SPDX-License-Identifier: GPL-2.0-or-later
import QtQuick 2.0
import org.shotcut.qml 1.0

Metadata {
    type: Metadata.Filter
    name: qsTr("360: Equirectangular Mask")
    mlt_service: "frei0r.bigsh0t_eq_mask"
    keywords: qsTr('spherical matte stencil', 'search keywords for the 360: Equirectangular Mask video filter') + ' 360: equirectangular mask bigsh0t'
    objectName: "bigsh0t_eq_mask"
    qml: "ui.qml"

    keyframes {
        allowAnimateIn: true
        allowAnimateOut: true
        simpleProperties: []
        parameters: [
            Parameter {
                name: qsTr('Horizontal Start')
                property: 'hfov0'
                isCurve: true
                minimum: 0
                maximum: 360
            },
            Parameter {
                name: qsTr('Horizontal End')
                property: 'hfov1'
                isCurve: true
                minimum: 0
                maximum: 360
            },
            Parameter {
                name: qsTr('Vertical Start')
                property: 'vfov0'
                isCurve: true
                minimum: 0
                maximum: 360
            },
            Parameter {
                name: qsTr('Vertical End')
                property: 'vfov1'
                isCurve: true
                minimum: 0
                maximum: 360
            }
        ]
    }

}
