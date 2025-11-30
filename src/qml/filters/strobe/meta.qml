import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    name: qsTr("Alpha Strobe")
    keywords: qsTr('strobe alpha blink', 'search keywords for the Strobe video filter') + ' strobe #rgba #10bit'
    mlt_service: "strobe"
    qml: "ui.qml"
    icon: 'icon.webp'

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
