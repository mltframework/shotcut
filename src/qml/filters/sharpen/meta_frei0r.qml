import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    name: qsTr("Sharpen")
    keywords: qsTr('sharpness focus clear crisp', 'search keywords for the Sharpen video filter') + ' sharpen #rgba'
    mlt_service: "frei0r.sharpness"
    qml: "ui_frei0r.qml"
    icon: 'icon.webp'
    gpuAlt: "movit.sharpen"

    keyframes {
        allowAnimateIn: true
        allowAnimateOut: true
        simpleProperties: ['0', '1']
        parameters: [
            Parameter {
                name: qsTr('Amount')
                property: '0'
                isCurve: true
                minimum: 0
                maximum: 1
            },
            Parameter {
                name: qsTr('Size')
                property: '1'
                isCurve: true
                minimum: 0
                maximum: 1
            }
        ]
    }
}
