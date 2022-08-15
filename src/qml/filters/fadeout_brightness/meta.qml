import QtQuick 2.0
import org.shotcut.qml 1.0

Metadata {
    type: Metadata.Filter
    objectName: 'fadeOutBrightness'
    name: qsTr("Fade Out Video")
    mlt_service: "brightness"
    qml: "ui.qml"
    isFavorite: true
    gpuAlt: "movit.opacity"
    allowMultiple: false

    keyframes {
        allowTrim: false
        allowAnimateOut: true
    }

}
