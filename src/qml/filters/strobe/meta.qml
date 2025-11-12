import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    name: qsTr("Alpha Strobe")
    keywords: qsTr('strobe alpha blink', 'search keywords for the Strobe video filter') + ' strobe #rgba'
    mlt_service: "strobe"
    qml: "ui.qml"

    keyframes {
        allowAnimateIn: true
        allowAnimateOut: true
        simpleProperties: ['interval']
        parameters: [
            Parameter {
                name: qsTr('Interval')
                property: 'interval'
                isCurve: true
                minimum: 0
                maximum: 100
            }
        ]
    }
}
