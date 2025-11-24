import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    objectName: 'fadeInMovit'
    name: qsTr("Fade In Video")
    keywords: qsTr('brightness lightness opacity alpha', 'search keywords for the Fade In video filter') + ' fade in video #gpu #10bit'
    mlt_service: "movit.opacity"
    needsGPU: true
    qml: "ui.qml"
    icon: 'icon.webp'
    isFavorite: true
    allowMultiple: false

    keyframes {
        allowTrim: false
        allowAnimateIn: true
    }
}
