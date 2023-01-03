import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    objectName: 'fadeOutMovit'
    name: qsTr("Fade Out Video")
    keywords: qsTr('brightness lightness opacity alpha', 'search keywords for the Fade Out video filter') + ' fade out video'
    mlt_service: "movit.opacity"
    needsGPU: true
    qml: "ui.qml"
    icon: 'icon.webp'
    isFavorite: true
    allowMultiple: false

    keyframes {
        allowTrim: false
        allowAnimateOut: true
    }
}
