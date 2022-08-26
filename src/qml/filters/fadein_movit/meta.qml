import QtQuick 2.0
import org.shotcut.qml 1.0

Metadata {
    type: Metadata.Filter
    objectName: 'fadeInMovit'
    name: qsTr("Fade In Video")
    keywords: qsTr('brightness lightness opacity alpha', 'search keywords for the Fade In video filter') + ' fade in video'
    mlt_service: "movit.opacity"
    needsGPU: true
    qml: "ui.qml"
    isFavorite: true
    allowMultiple: false

    keyframes {
        allowTrim: false
        allowAnimateIn: true
    }

}
