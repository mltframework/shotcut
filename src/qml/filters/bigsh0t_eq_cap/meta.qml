/* SPDX-License-Identifier: GPL-2.0-or-later */
import QtQuick 2.0
import org.shotcut.qml 1.0

Metadata {
    type: Metadata.Filter
    name: qsTr("360: Cap Top & Bottom")
    mlt_service: "frei0r.bigsh0t_eq_cap"
    keywords: qsTr('spherical fill blur zenith nadir', 'search keywords for the 360: Cap Top & Bottom video filter') + ' 360: cap top & bottom bigsh0t'
    objectName: "bigsh0t_eq_cap"
    qml: "ui.qml"
    help: 'https://forum.shotcut.org/t/350-cap-top-bottom-video-filter/48129/1'

    keyframes {
        allowAnimateIn: true
        allowAnimateOut: true
        simpleProperties: [
            "topStart",
            "topEnd",
            "topBlendIn",
            "topBlendOut",
            "topFadeIn",
            "topBlurWidthStart",
            "topBlurWidthEnd",
            "topBlurHeightStart",
            "topBlurHeightEnd",

            "bottomStart",
            "bottomEnd",
            "bottomBlendIn",
            "bottomBlendOut",
            "bottomFadeIn",
            "bottomBlurWidthStart",
            "bottomBlurWidthEnd",
            "bottomBlurHeightStart",
            "bottomBlurHeightEnd"
        ]
        parameters: [
            Parameter {
                name: qsTr('topStart')
                property: 'topStart'
                isCurve: true
                minimum: 0
                maximum: 90
            },
            Parameter {
                name: qsTr('topEnd')
                property: 'topEnd'
                isCurve: true
                minimum: 0
                maximum: 90
            },
            Parameter {
                name: qsTr('topBlendIn')
                property: 'topBlendIn'
                isCurve: true
                minimum: 0
                maximum: 90
            },
            Parameter {
                name: qsTr('topBlendOut')
                property: 'topBlendOut'
                isCurve: true
                minimum: 0
                maximum: 90
            },
            Parameter {
                name: qsTr('topFadeIn')
                property: 'topFadeIn'
                isCurve: true
                minimum: 0
                maximum: 90
            },
            Parameter {
                name: qsTr('topBlurWidthStart')
                property: 'topBlurWidthStart'
                isCurve: true
                minimum: 0
                maximum: 360
            },
            Parameter {
                name: qsTr('topBlurWidthEnd')
                property: 'topBlurWidthEnd'
                isCurve: true
                minimum: 0
                maximum: 360
            },
            Parameter {
                name: qsTr('topBlurHeightStart')
                property: 'topBlurHeightStart'
                isCurve: true
                minimum: 0
                maximum: 90
            },
            Parameter {
                name: qsTr('topBlurHeightEnd')
                property: 'topBlurHeightEnd'
                isCurve: true
                minimum: 0
                maximum: 90
            },

            Parameter {
                name: qsTr('bottomStart')
                property: 'bottomStart'
                isCurve: true
                minimum: 0
                maximum: 90
            },
            Parameter {
                name: qsTr('bottomEnd')
                property: 'bottomEnd'
                isCurve: true
                minimum: 0
                maximum: 90
            },
            Parameter {
                name: qsTr('bottomBlendIn')
                property: 'bottomBlendIn'
                isCurve: true
                minimum: 0
                maximum: 90
            },
            Parameter {
                name: qsTr('bottomBlendOut')
                property: 'bottomBlendOut'
                isCurve: true
                minimum: 0
                maximum: 90
            },
            Parameter {
                name: qsTr('bottomFadeIn')
                property: 'bottomFadeIn'
                isCurve: true
                minimum: 0
                maximum: 90
            },
            Parameter {
                name: qsTr('bottomBlurWidthStart')
                property: 'bottomBlurWidthStart'
                isCurve: true
                minimum: 0
                maximum: 360
            },
            Parameter {
                name: qsTr('bottomBlurWidthEnd')
                property: 'bottomBlurWidthEnd'
                isCurve: true
                minimum: 0
                maximum: 360
            },
            Parameter {
                name: qsTr('bottomBlurHeightStart')
                property: 'bottomBlurHeightStart'
                isCurve: true
                minimum: 0
                maximum: 90
            },
            Parameter {
                name: qsTr('bottomBlurHeightEnd')
                property: 'bottomBlurHeightEnd'
                isCurve: true
                minimum: 0
                maximum: 90
            }
        ]
    }
}
