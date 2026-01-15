import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    name: qsTr("LUT (3D)")
    keywords: qsTr('lookup table color', 'search keywords for the LUT (3D) video filter') + ' 3dl cube dat m3d lut (3d) #rgba #10bit #color'
    mlt_service: 'avfilter.lut3d'
    qml: 'ui.qml'
    icon: 'icon.webp'
    help: 'https://forum.shotcut.org/t/lut-3d/12858/1'
}
