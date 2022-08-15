// SPDX-License-Identifier: GPL-2.0-or-later
import QtQuick 2.0
import org.shotcut.qml 1.0

Metadata {
    type: Metadata.Filter
    name: qsTr("360: Rectilinear to Equirectangular")
    mlt_service: "frei0r.bigsh0t_rect_to_eq"
    objectName: "bigsh0t_rect_to_eq"
    qml: "ui.qml"

    keyframes {
        allowAnimateIn: true
        allowAnimateOut: true
        simpleProperties: ["hfov", "vfov"]
        parameters: [
            Parameter {
                name: qsTr('Horizontal')
                property: 'hfov'
                isCurve: true
                minimum: 0
                maximum: 180
            },
            Parameter {
                name: qsTr('Vertical')
                property: 'vfov'
                isCurve: true
                minimum: 0
                maximum: 180
            }
        ]
    }

}
