import QtQuick 2.0
import org.shotcut.qml 1.0

Metadata {
    type: Metadata.Filter
    name: qsTr("Posterize")
    keywords: qsTr('reduce colors banding cartoon', 'search keywords for the Posterize video filter') + ' posterize'
    objectName: 'posterize'
    mlt_service: "frei0r.posterize"
    qml: "ui.qml"
    icon: 'icon.qml'

    keyframes {
        allowAnimateIn: true
        allowAnimateOut: true
        simpleProperties: ['0']
        parameters: [
            Parameter {
                name: qsTr('Levels', 'Posterize filter')
                property: '0'
                isCurve: true
                minimum: 0
                maximum: 1
            }
        ]
    }

}
