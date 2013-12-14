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
            width: clipDuration * timeScale
            height: trackRoot.height
            trackIndex: trackRoot.DelegateModel.itemsIndex
            onSelected: {
                resetStates(clip.DelegateModel.itemsIndex);
                trackRoot.clipSelected(clip, trackRoot);
            }
            onMoved: {
//                console.log('clip moved: ' + clip.DelegateModel.itemsIndex + ' position ' + Math.round(newX / timeScale))
                for (var i = repeater.count - 1; i >= 0; i--) {
                    if (i !== clip.DelegateModel.itemsIndex && newX >= repeater.itemAt(i).originalX) {
                        console.log('dropped onto clip ' + i + ': ' + repeater.itemAt(i).clipName)
                        break
                    }
                }
            }
            onDragged: {
                var mapped = trackRoot.mapFromItem(repeater.itemAt(clip.DelegateModel.itemsIndex), mouse.x, mouse.y)
                trackRoot.clipDragged(clip, mapped.x, mapped.y)
            }
            onTrimmedIn: {
                timeline.position = clip.x / timeScale
                // Adjust width of clip left of argument.
                var i = clip.DelegateModel.itemsIndex - 1;
                // TODO handle left-most item
                if (i >= 0)
                    repeater.itemAt(i).clipDuration += delta;
            }
            onTrimmedOut: {
                timeline.position = (clip.x + clip.width) / timeScale
                // Adjust width of clip right of argument.
                var i = clip.DelegateModel.itemsIndex + 1;
                // TODO handle right-most item
                if (i < repeater.count)
                    repeater.itemAt(i).clipDuration += delta;
            }

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
