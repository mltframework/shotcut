import QtQuick 2.0
import org.shotcut.qml 1.0

Metadata {
    type: Metadata.Filter
    objectName: 'fadeOutMovit'
    name: qsTr("Fade Out Video")
    keywords: qsTr('brightness lightness opacity alpha', 'search keywords for the Fade Out video filter') + ' fade out video'
    mlt_service: "movit.opacity"
    needsGPU: true
    qml: "ui.qml"
    isFavorite: true
    allowMultiple: false

    keyframes {
        allowTrim: false
        allowAnimateOut: true
    }

}
