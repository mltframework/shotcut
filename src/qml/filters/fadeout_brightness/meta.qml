import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    objectName: 'fadeOutBrightness'
    name: qsTr("Fade Out Video")
    keywords: qsTr('brightness lightness opacity alpha', 'search keywords for the Fade Out video filter') + ' fade out video #yuv'
    mlt_service: "brightness"
    qml: "ui.qml"
    icon: 'icon.webp'
    isFavorite: true
    gpuAlt: "movit.opacity"
    allowMultiple: false

    keyframes {
        allowTrim: false
        allowAnimateOut: true
    }
}
