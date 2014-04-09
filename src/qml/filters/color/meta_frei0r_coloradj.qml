import QtQuick 2.0
import org.shotcut.qml 1.0

Metadata {
    type: Metadata.Filter
    objectName: "Frei0rColoradjWidget"
    name: qsTr("Color Grading")
    mlt_service: "frei0r.coloradj_RGB"
    qml: "ui_frei0r_coloradj.qml"
    isHidden: true
}
