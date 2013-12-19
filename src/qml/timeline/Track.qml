import QtQuick 2.0
import QtQml.Models 2.1

Rectangle {
    id: trackRoot
    property alias model: trackModel.model
    property alias rootIndex: trackModel.rootIndex
    property bool isAudio
    property real timeScale: 1.0

    signal clipSelected(var clip, var track)
    signal clipDragged(var clip, int x, int y)
    signal clipDropped(var clip)
    signal clipDraggedToTrack(var clip, int direction)

    function resetStates(index) {
        for (var i = 0; i < repeater.count; i++)
            if (i !== index) repeater.itemAt(i).state = ''
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
            height: trackRoot.height
            trackIndex: trackRoot.DelegateModel.itemsIndex
            onSelected: {
                resetStates(clip.DelegateModel.itemsIndex);
                trackRoot.clipSelected(clip, trackRoot);
            }
            onMoved: {
                if (!multitrack.moveClip(trackRoot.DelegateModel.itemsIndex, clip.DelegateModel.itemsIndex, Math.round(clip.x / timeScale)))
                    clip.x = clip.originalX
            }
            onDragged: {
                var mapped = trackRoot.mapFromItem(repeater.itemAt(clip.DelegateModel.itemsIndex), mouse.x, mouse.y)
                trackRoot.clipDragged(clip, mapped.x, mapped.y)
            }
            onTrimmingIn: multitrack.trimClipIn(trackRoot.DelegateModel.itemsIndex, clip.DelegateModel.itemsIndex, delta)
            onTrimmedIn:multitrack.notifyClipIn(trackRoot.DelegateModel.itemsIndex, clip.DelegateModel.itemsIndex)
            onTrimmingOut: multitrack.trimClipOut(trackRoot.DelegateModel.itemsIndex, clip.DelegateModel.itemsIndex, delta)
            onTrimmedOut: multitrack.notifyClipOut(trackRoot.DelegateModel.itemsIndex, clip.DelegateModel.itemsIndex)

            Component.onCompleted: {
                moved.connect(trackRoot.clipDropped)
                dropped.connect(trackRoot.clipDropped)
                draggedToTrack.connect(trackRoot.clipDraggedToTrack)
            }
        }
    }

    Row {
        Repeater { id: repeater; model: trackModel }
    }
}
