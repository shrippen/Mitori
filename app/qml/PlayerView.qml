pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import QtMultimedia
import org.kde.kirigami as Kirigami
import Mitori

// The player (clickdummy "Player" and "Querformat"): the technique's chapter of the exam video,
// without the black bars, in a loop by default; the style's transport bar with four tempi (sound off
// below 100 %) and single frames; double tap left/right = 2 s back/on. Landscape: the controls in
// a column on the right. PlayerPage shows it as a page; on a tablet the programme shows it beside
// the list (`compact`: picture above the controls, no Back). Swipe up/down on the left half of
// the picture: the screen's brightness, on the right half: the volume (Levels); the picture's
// height is the whole range. Leaving the player gives the screen its brightness back.
Item {
    id: page

    required property var playlist
    property int index: 0
    // Beside the list on a tablet: always the picture above the controls, no Back button.
    property bool compact: false
    readonly property var current: playlist[index] || ({})
    readonly property string video: current.video ? current.video.video : ""
    // A technique of the training may have no video on the device: a card stands for the picture.
    readonly property bool hasVideo: video !== "" && Videos.available(video)
    // Where the technique stands: attack · addition · grade · position ("Shomen-uchi · 5. Kyu · 3 / 8").
    readonly property string placeText: [current.attack, current.attackExtra, current.gradeName,
                                         playlist.length > 1 ? (index + 1) + " / " + playlist.length : ""]
                                        .filter(x => !!x).join(" · ")
    // How many of the playlist are rated today (the training's progress).
    readonly property int practisedCount: Practice.revision >= 0 ? playlist.filter(r => Practice.ratedToday(r.id)).length : 0
    readonly property real from: current.video ? current.video.von * 1000 : 0
    readonly property real to: current.video ? current.video.bis * 1000 : 0
    // Where the loop turns: the chapter end, but never the end of the file (the last chapter of a
    // video ends with it, and a player at the end of the file no longer seeks back).
    readonly property real end: player.duration > 0 ? Math.min(to, player.duration - 250) : to
    readonly property var crop: Videos.crop(video)
    // Sideways only with a picture; without a video the card and the controls stand as upright.
    readonly property bool landscape: !compact && width > height && hasVideo
    readonly property var tempi: [0.25, 0.5, 0.75, 1]
    readonly property int frameMs: 40       // 25 frames per second (PAL DVDs)
    readonly property int jumpMs: 2000      // double tap
    property bool looping: true
    // Training (today's practice): rating a technique moves on to the next one not yet
    // practised today.
    property bool training: false
    // Start at the chapter once per loaded file: the FFmpeg backend reports LoadedMedia again
    // after every seek, and restarting there would undo each step, jump and timeline move.
    property bool startPending: true
    // Whether a newly loaded chapter plays (off while another page lies above the small player:
    // it loads at the chapter start and waits).
    property bool autoPlay: true
    // For the UI test: where the video stands (ms) and whether it plays.
    readonly property alias position: player.position
    readonly property alias playing: player.playing
    readonly property alias duration: player.duration
    readonly property alias rate: player.playbackRate

    // The entry on screen, for the programme list to mark it when the user comes back.
    signal shown(string id)
    // Back (landscape, training done) and the notes of the technique: the page decides.
    signal backRequested()
    signal notesRequested(var entry)
    signal commentRequested(var entry)

    onCurrentChanged: {
        shown(current.id)
        // After the bindings of the new entry (from, to): onCurrentChanged runs before them.
        if (player.source.toString() === Videos.source(video).toString() && !startPending) {
            Qt.callLater(page.restart)
        }
    }
    Component.onCompleted: shown(current.id)
    Component.onDestruction: {
        player.stop()
        Levels.restore()
    }

    function restart() {
        player.position = from
        if (autoPlay) {
            player.play()
        }
    }
    // Play from the chapter start when the video stands at (or after) the chapter end.
    function togglePlay() {
        if (player.playing) {
            player.pause()
        } else if (player.position >= end - frameMs || player.mediaStatus === MediaPlayer.EndOfMedia) {
            restart()
        } else {
            player.play()
        }
    }
    function pause() {
        player.pause()
    }
    function step(direction) {
        player.pause()
        player.position = Math.max(from, Math.min(to, player.position + direction * frameMs))
    }
    function jump(ms) {
        player.position = Math.max(from, Math.min(to, player.position + ms))
    }
    // Training: the next technique not practised today, after this one and then from the start
    // (one skipped with "next"); when all are, the training is done and the player stays.
    readonly property bool trainingDone: training && Practice.revision >= 0 && playlist.every(r => Practice.ratedToday(r.id))
    function nextOpen() {
        for (let k = 1; k < playlist.length; ++k) {
            const i = (index + k) % playlist.length
            if (!Practice.ratedToday(playlist[i].id)) {
                index = i
                return
            }
        }
    }
    function go(direction) {
        const next = index + direction
        if (next >= 0 && next < playlist.length) {
            index = next
        }
    }
    function timeText(ms) {
        const s = Math.max(0, Math.round(ms / 100) / 10)
        return s.toLocaleString(Qt.locale(), "f", 1) + " s"
    }

    MediaPlayer {
        id: player

        source: Videos.source(page.video)
        videoOutput: output
        audioOutput: AudioOutput {
            muted: player.playbackRate < 1 || !Levels.sound
            volume: Levels.playerVolume
        }
        playbackRate: 1
        // The screen stays on while it plays (the switch "Screen on"), not while it stands.
        onPlayingChanged: Levels.setPlayback(playing ? Levels.Playback.Playing : Levels.Playback.Stopped)
        onSourceChanged: page.startPending = true
        onMediaStatusChanged: {
            if (player.mediaStatus === MediaPlayer.LoadedMedia && page.startPending) {
                page.startPending = false
                page.restart()
            }
        }
        onPositionChanged: {
            if (page.to <= 0 || player.position < page.end) {
                return
            }
            if (page.looping) {
                player.position = page.from
            } else {
                player.pause()
            }
        }
    }

    // The bar's keys (K, L, Home, + −, , .) work as soon as the player opens, not only after a
    // click into the bar; it lets Space and the arrows through to the handlers below.
    Keys.forwardTo: [transport]
    Keys.onSpacePressed: page.togglePlay()
    Keys.onLeftPressed: page.step(-1)
    Keys.onRightPressed: page.step(1)

    GridLayout {
        anchors.fill: parent
        columns: page.landscape ? 2 : 1
        rowSpacing: 0
        columnSpacing: 0

        // The picture: VideoOutput taller than the stage and shifted up, the stage clips the bars.
        Item {
            id: stage

            objectName: "stage"
            visible: page.hasVideo
            readonly property real ratio: page.crop.verhaeltnis || 16 / 9
            Layout.fillWidth: true
            Layout.fillHeight: page.landscape
            Layout.preferredHeight: page.landscape ? -1 : width / ratio
            // Beside the list the controls keep the room they need, on a small tablet half the height.
            Layout.maximumHeight: page.compact ? Math.max(page.height / 2, page.height - controlsView.neededHeight)
                                               : Number.POSITIVE_INFINITY

            Rectangle {
                anchors.fill: parent
                color: Look.stageColor
            }

            Item {
                id: picture

                anchors.centerIn: parent
                width: Math.min(parent.width, parent.height * stage.ratio)
                height: width / stage.ratio
                clip: true

                VideoOutput {
                    id: output

                    readonly property real factor: page.crop.h ? picture.height / page.crop.h : 1
                    width: picture.width
                    height: page.crop.hoehe ? page.crop.hoehe * factor : picture.height
                    y: page.crop.y ? -page.crop.y * factor : 0
                    fillMode: VideoOutput.Stretch
                }
            }

            QQC2.Label {
                visible: page.landscape
                anchors.left: parent.left
                anchors.top: parent.top
                anchors.margins: Kirigami.Units.largeSpacing
                text: page.current.title + (page.current.attack ? "  ·  " + page.current.attack : "")
                color: Look.stageTextColor
                style: Text.Outline
                styleColor: Look.stageColor
            }

            // Swipe: brightness (left half) or volume (right half), shown while the finger moves.
            DragHandler {
                id: swipe

                property int level: Levels.Level.Brightness
                property real lastY: 0

                target: null
                xAxis.enabled: false
                onActiveChanged: {
                    if (!active) {
                        return
                    }
                    Levels.refresh()
                    level = centroid.pressPosition.x < stage.width / 2 ? Levels.Level.Brightness : Levels.Level.Volume
                    lastY = 0
                }
                onTranslationChanged: {
                    if (!active) {
                        return
                    }
                    const dy = translation.y - lastY
                    lastY = translation.y
                    Levels.change(level, -dy / stage.height)
                }
            }

            ColumnLayout {
                objectName: "levelShown"
                anchors.centerIn: parent
                width: Math.min(parent.width / 2, Kirigami.Units.gridUnit * 12)
                visible: swipe.active && value >= 0
                readonly property real value: swipe.level === Levels.Level.Brightness ? Levels.brightness : Levels.volume

                QQC2.Label {
                    Layout.alignment: Qt.AlignHCenter
                    text: (swipe.level === Levels.Level.Brightness ? qsTr("Brightness %1 %") : qsTr("Volume %1 %"))
                          .arg(Math.round(parent.value * 100))
                    color: Look.stageTextColor
                    style: Text.Outline
                    styleColor: Look.stageColor
                }
                LookProgressBar {
                    Layout.fillWidth: true
                    value: parent.value
                }
            }

            TapHandler {
                onTapped: page.togglePlay()
                onDoubleTapped: (eventPoint, button) => page.jump(eventPoint.position.x < stage.width / 2 ? -page.jumpMs : page.jumpMs)
            }
        }

        // The controls scroll when they do not fit (a phone held sideways: rating and Back).
        Flickable {
            id: controlsView

            objectName: "controlsView"
            Layout.fillWidth: !page.landscape
            Layout.fillHeight: true             // portrait: the picture stays at the top
            Layout.preferredWidth: page.landscape ? Kirigami.Units.gridUnit * 17 : -1     // room for the tempo row
            contentHeight: controls.implicitHeight + Kirigami.Units.largeSpacing * 2
            // What the controls need without the positions of a sequence (they scroll below).
            readonly property real neededHeight: contentHeight
                                                 - (stepList.visible ? stepList.height + stepList.Layout.topMargin + controls.spacing : 0)
            boundsBehavior: Flickable.StopAtBounds
            clip: true
            QQC2.ScrollBar.vertical: QQC2.ScrollBar {}

            // The note field while it has the focus (NoteRow), kept in view as the height changes.
            property Item focusedField: null
            function showFocusedField() {
                if (!focusedField || !focusedField.activeFocus) {
                    return
                }
                const y = focusedField.mapToItem(controls, 0, 0).y
                contentY = Math.max(0, Math.min(contentHeight - height,
                                                y - height + focusedField.height + Kirigami.Units.largeSpacing))
            }
            onHeightChanged: Qt.callLater(showFocusedField)
            // Tab below the edge (the small player beside the list): the column follows the focus.
            readonly property LookFocusFollow focusFollow: LookFocusFollow { flickable: controlsView }

            ColumnLayout {
                id: controls

                x: Kirigami.Units.largeSpacing
                y: Kirigami.Units.largeSpacing
                width: controlsView.width - Kirigami.Units.largeSpacing * 2
                spacing: Kirigami.Units.largeSpacing

                QQC2.Label {
                    Layout.fillWidth: true
                    visible: page.compact
                    text: (page.current.nr ? page.current.nr + " · " : "") + (page.current.title || "")
                    font.bold: true
                    wrapMode: Text.Wrap
                }

                // First, so a phone held sideways shows them without scrolling the column.
                LookToolButton {
                    objectName: "backButton"
                    visible: page.landscape
                    icon.name: "go-previous-view"
                    text: qsTr("Back")
                    onClicked: page.backRequested()
                }

                LookHint {
                    objectName: "trainingDone"
                    Layout.fillWidth: true
                    visible: page.trainingDone
                    importance: LookHint.Importance.Info
                    titleText: qsTr("Done")
                    text: qsTr("%n technique(s) practised today. They count as a practice day in the statistics.", "", page.practisedCount)

                    LookButton {
                        visible: !page.compact && !page.landscape     // sideways Back stands above
                        text: qsTr("Back")
                        onClicked: page.backRequested()
                    }
                }

                QQC2.Label {
                    objectName: "placeText"
                    Layout.fillWidth: true
                    visible: !page.landscape && page.placeText !== ""
                    text: page.placeText
                    color: Look.disabledTextColor
                    wrapMode: Text.Wrap
                }

                // Without a video: the technique's name and what to do (the training goes on).
                LookHint {
                    objectName: "noVideoCard"
                    Layout.fillWidth: true
                    visible: !page.hasVideo
                    titleText: (page.current.nr ? page.current.nr + " · " : "") + (page.current.title || "")
                    // No video at all (the higher grades) or one not loaded on this device.
                    text: page.video === "" ? qsTr("There is no video of this technique. Practise it from memory, then rate it.")
                                            : qsTr("Its video is not on this device. Practise it from memory, then rate it.")
                }

                QQC2.Slider {
                    id: timeline

                    Layout.fillWidth: true
                    visible: page.hasVideo
                    from: page.from
                    to: page.to > page.from ? page.to : page.from + 1
                    value: player.position
                    onMoved: player.position = value
                    Accessible.name: qsTr("Position")

                    LookSliderSkin { control: timeline }
                }

                LookTransportBar {
                    id: transport

                    objectName: "transportBar"
                    Layout.fillWidth: true
                    visible: page.hasVideo
                    Layout.preferredWidth: 0        // wraps in the column instead of widening it
                    playing: player.playing
                    looping: page.looping
                    speed: player.playbackRate
                    speedSteps: page.tempi
                    metronomeVisible: false
                    countInVisible: false
                    frameStep: true
                    positionText: page.timeText(player.position - page.from) + " / " + page.timeText(page.to - page.from)
                    rewindText: qsTr("Chapter start")
                    playText: qsTr("Play")
                    pauseText: qsTr("Pause")
                    loopText: qsTr("Loop")
                    speedText: qsTr("Tempo")
                    frameBackText: qsTr("Frame back")
                    frameOnText: qsTr("Frame on")
                    positionName: qsTr("Position")
                    accessibleName: qsTr("Playback")
                    onPlayToggled: page.togglePlay()
                    onLoopToggled: page.looping = !page.looping
                    onSpeedChangeRequested: speed => player.playbackRate = speed
                    onRewind: player.position = page.from
                    onFrameStepRequested: direction => page.step(direction)
                }

                // The player's switches, kept for the next start: sound (always off in slow
                // motion) and the screen staying on while a video plays.
                RowLayout {
                    Layout.fillWidth: true
                    visible: page.hasVideo

                    LookToolButton {
                        objectName: "soundSwitch"
                        // On only where it sounds: slow motion is always silent.
                        readonly property bool sounds: Levels.sound && player.playbackRate >= 1
                        enabled: player.playbackRate >= 1     // a tap in slow motion would change it unseen
                        checkable: true
                        checked: sounds
                        icon.name: sounds ? "audio-volume-high" : "audio-volume-muted"
                        text: player.playbackRate < 1 ? qsTr("Silent in slow motion") : qsTr("Sound")
                        onClicked: {
                            checked = Qt.binding(() => sounds)
                            Levels.setSound(Levels.sound ? Levels.Switch.Off : Levels.Switch.On)
                        }
                    }
                    LookToolButton {
                        objectName: "awakeSwitch"
                        checkable: true
                        checked: Levels.awake
                        icon.name: "brightness-high"
                        text: qsTr("Screen on")
                        onClicked: Levels.setAwake(Levels.awake ? Levels.Switch.Off : Levels.Switch.On)
                    }
                }

                RatingRow {
                    Layout.fillWidth: true
                    entry: page.current.id || ""
                    onRated: if (page.training) page.nextOpen()
                }

                // Beside the list on a tablet the sheet does not open: the comment is here.
                LookButton {
                    objectName: "commentButton"
                    visible: page.compact && !!page.current.comment
                    icon.name: "documentinfo"
                    text: qsTr("Comment")
                    onClicked: page.commentRequested(page.current)
                }

                NoteRow {
                    Layout.fillWidth: true
                    entry: page.current
                    onNotesRequested: page.notesRequested(page.current)
                    // Above the on-screen keyboard: scroll the field into view when it gets the
                    // focus and again when the keyboard opens (the window shrinks: adjustResize).
                    onFieldFocused: field => {
                        controlsView.focusedField = field
                        Qt.callLater(controlsView.showFocusedField)
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    visible: !page.landscape

                    LookToolButton {
                        enabled: page.index > 0
                        icon.name: "media-skip-backward"
                        text: qsTr("Previous technique")
                        onClicked: page.go(-1)
                    }
                    Item {
                        Layout.fillWidth: true
                    }
                    LookToolButton {
                        enabled: page.index < page.playlist.length - 1
                        icon.name: "media-skip-forward"
                        text: qsTr("Next technique")
                        onClicked: page.go(1)
                    }
                }

                // The training's playlist: numbered, ✓ for the rated ones, a tap jumps there.
                ColumnLayout {
                    objectName: "trainingList"
                    Layout.fillWidth: true
                    visible: page.training
                    spacing: 0

                    LookSectionLabel {
                        Layout.fillWidth: true
                        rule: true
                        text: qsTr("Today's playlist · %1 of %2 practised").arg(page.practisedCount).arg(page.playlist.length)
                    }
                    Repeater {
                        model: page.training ? page.playlist : []

                        LookListRow {
                            id: trainingRow

                            required property var modelData
                            required property int index
                            readonly property bool done: Practice.revision >= 0 && Practice.ratedToday(modelData.id)

                            objectName: "trainingRow"
                            Layout.fillWidth: true
                            focusOnClick: false     // Space stays with the player (play, pause)
                            selected: trainingRow.index === page.index
                            text: (trainingRow.index + 1) + ". " + (modelData.title || "")
                            subtitle: [modelData.attack, modelData.gradeName].filter(x => !!x).join(" · ")
                            trailing: QQC2.Label {
                                text: trainingRow.done ? "✓" : ""
                                color: Look.positiveTextColor
                                Accessible.name: trainingRow.done ? qsTr("practised") : ""
                            }
                            onClicked: page.index = trainingRow.index
                        }
                    }
                }

                // The positions of a sequence (Kamae exercise): beside the list on a tablet no
                // sheet opens, and in the player they go along with the video. Last, so "Done",
                // the way on and Back stay in view; the picture does not give way to them.
                StepList {
                    id: stepList

                    Layout.fillWidth: true
                    Layout.topMargin: Kirigami.Units.largeSpacing
                    steps: page.current.steps || []
                }
            }
        }
    }
}
