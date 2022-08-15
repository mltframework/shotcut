import QtQuick 2.0
import org.shotcut.qml 1.0

Metadata {
    type: Metadata.Filter
    objectName: 'richText'
    name: qsTr('Text: Rich')
    mlt_service: 'qtext'
    qml: "ui.qml"
    vui: 'vui.qml'
    isFavorite: true
    minimumVersion: '2'

    keyframes {
        allowAnimateIn: true
        allowAnimateOut: true
        simpleProperties: ['geometry']
        parameters: [
            Parameter {
                name: qsTr('Position / Size')
                property: 'geometry'
                isRectangle: true
            }
        ]
    }

}
