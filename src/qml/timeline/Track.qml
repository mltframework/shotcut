import QtQuick 2.0
import QtQml.Models 2.1

Column {
    property alias model: trackModel.model
    property alias rootIndex: trackModel.rootIndex
    property color color

    DelegateModel {
        id: trackModel
        Clip {
            clipName: name
            clipResource: resource
            clipDuration: duration
            isBlank: blank
            isAudio: audio
        }
    }

    Row {
        Repeater { model: trackModel }
    }
}
