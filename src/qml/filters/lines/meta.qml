import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    name: qsTr("Old Film: Scratches")
    keywords: qsTr('noise projector lines defect', 'search keywords for the Old Film: Scratches video filter') + ' old film: scratches #yuv'
    mlt_service: "lines"
    qml: "ui.qml"
    icon: 'icon.webp'
}
