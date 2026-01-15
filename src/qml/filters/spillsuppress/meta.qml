import QtQuick
import org.shotcut.qml

Metadata {
    type: Metadata.Filter
    name: qsTr("Key Spill: Simple")
    keywords: qsTr('chroma alpha clean suppress', 'search keywords for the Key Spill: Simple video filter') + ' key spill: simple #rgba'
    mlt_service: 'frei0r.spillsupress'
    qml: 'ui.qml'
    help: 'https://forum.shotcut.org/t/key-spill-simple/12855/1'
}
