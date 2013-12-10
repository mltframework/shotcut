import QtQuick 2.0
import QtQml.Models 2.1

Rectangle {
    id: trackTop
    property alias model: trackModel.model
    property alias rootIndex: trackModel.rootIndex
    property real timeScale: 1.0
    property int trackIndex;

    signal clipSelected(int trackIndex)

    function resetStates(index) {
        for (var i = 0; i < repeater.count; i++)
            if (i != index) repeater.itemAt(i).state = ''
    }

    color: 'transparent'

    DelegateModel {
        id: trackModel
        Clip {
            clipName: model.name
            clipResource: model.resource
            clipDuration: model.duration
            mltService: model.mlt_service
            inPoint: model.in
            outPoint: model.out
            isBlank: model.blank
            isAudio: model.audio
            audioLevels: model.audioLevels
            width: model.duration * timeScale
            height: trackTop.height
            clipIndex: index
            onClipSelected: {
                resetStates(clipIndex);
                trackTop.clipSelected(trackIndex);
            }
        }
    }

    Row {
        Repeater { id: repeater; model: trackModel }
    }
}
