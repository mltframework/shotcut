import QtQuick 2.0
import QtQml.Models 2.1

Rectangle {
    id: trackTop
    property alias model: trackModel.model
    property alias rootIndex: trackModel.rootIndex
    property real timeScale: 1.0

    DelegateModel {
        id: trackModel
        Clip {
            clipName: name
            clipResource: resource
            clipDuration: duration
            mltService: mlt_service
            inPoint: model.in
            outPoint: model.out
            isBlank: blank
            isAudio: audio
            width: duration * timeScale
            height: trackTop.height
        }
    }

    Row {
        Repeater { model: trackModel }
    }
}
