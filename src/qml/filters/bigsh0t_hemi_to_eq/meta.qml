// SPDX-License-Identifier: GPL-2.0-or-later
import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    name: qsTr("360: Hemispherical to Equirectangular")
    mlt_service: "frei0r.bigsh0t_hemi_to_eq"
    keywords: qsTr('spherical projection dual fisheye', 'search keywords for the 360: Hemispherical to Equirectangular video filter') + ' 360: hemispherical to equirectangular bigsh0t #rgba'
    objectName: "bigsh0t_hemi_to_eq"
    qml: "ui.qml"
    icon: "icon.webp"
    help: 'https://forum.shotcut.org/t/360-hemispherical-to-equirectangular-video-filter/19167/1'

    keyframes {
        allowAnimateIn: true
        allowAnimateOut: true
        simpleProperties: ['yaw', "pitch", "roll", "frontX", "frontY", "frontUp", "backX", "backY", "backUp", "fov", "radius", "nadirRadius", "nadirCorrectionStart"]
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
                maximum: 360
            },
            Parameter {
                name: qsTr('Radius')
                property: 'radius'
                isCurve: true
                minimum: 0
                maximum: 1
            },
            Parameter {
                name: qsTr('Front X')
                property: 'frontX'
                isCurve: true
                minimum: 0
                maximum: 1
            },
            Parameter {
                name: qsTr('Front Y')
                property: 'frontY'
                isCurve: true
                minimum: 0
                maximum: 1
            },
            Parameter {
                name: qsTr('Front Up')
                property: 'frontUp'
                isCurve: true
                minimum: -360
                maximum: 360
            },
            Parameter {
                name: qsTr('Back X')
                property: 'backX'
                isCurve: true
                minimum: 0
                maximum: 1
            },
            Parameter {
                name: qsTr('Back Y')
                property: 'backY'
                isCurve: true
                minimum: 0
                maximum: 1
            },
            Parameter {
                name: qsTr('Back Up')
                property: 'backUp'
                isCurve: true
                minimum: -360
                maximum: 360
            },
            Parameter {
                name: qsTr('Nadir Radius')
                property: 'nadirRadius'
                isCurve: true
                minimum: 0
                maximum: 1
            },
            Parameter {
                name: qsTr('Nadir Start')
                property: 'nadirCorrectionStart'
                isCurve: true
                minimum: 0
                maximum: 1
            }
        ]
    }
}
