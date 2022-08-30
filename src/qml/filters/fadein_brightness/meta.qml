import QtQuick 2.0
import org.shotcut.qml 1.0

Metadata {
    type: Metadata.Filter
    objectName: 'fadeInBrightness'
    name: qsTr("Fade In Video")
    keywords: qsTr('brightness lightness opacity alpha', 'search keywords for the Fade In video filter') + ' fade in video'
    mlt_service: "brightness"
    qml: "ui.qml"
    icon: 'icon.webp'
    isFavorite: true
    gpuAlt: "movit.opacity"
    allowMultiple: false

    keyframes {
        allowTrim: false
        allowAnimateIn: true
    }

}
