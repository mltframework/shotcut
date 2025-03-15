// SPDX-License-Identifier: GPL-2.0-or-later
import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    name: qsTr("360: Rectilinear to Equirectangular")
    keywords: qsTr('spherical projection', 'search keywords for the 360: Rectilinear to Equirectangular video filter') + ' 360: rectilinear equirectangular bigsh0t #rgba'
    mlt_service: "frei0r.bigsh0t_rect_to_eq"
    objectName: "bigsh0t_rect_to_eq"
    qml: "ui.qml"
    icon: "icon.webp"

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
