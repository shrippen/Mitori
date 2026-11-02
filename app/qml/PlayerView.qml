import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import QtMultimedia
import org.kde.kirigami as Kirigami
import Kante
import Mitori

// The player (clickdummy "Player" and "Querformat"): the technique's chapter of the exam video,
// without the black bars, in a loop by default; Kante's transport bar with four tempi (sound off
// below 100 %) and single frames; double tap left/right = 2 s back/on. Landscape: the controls in
// a column on the right. PlayerPage shows it as a page; on a tablet the programme shows it beside
// the list (`compact`: picture above the controls, no Back). Brightness and volume gestures come
// with the platform layer (ROADMAP.md).
Item {
    id: page

    required property var playlist
    property int index: 0
    // Beside the list on a tablet: always the picture above the controls, no Back button.
    property bool compact: false
    readonly property var current: playlist[index] || ({})
    readonly property string video: current.video ? current.video.video : ""
    readonly property real from: current.video ? current.video.von * 1000 : 0
    readonly property real to: current.video ? current.video.bis * 1000 : 0
    // Where the loop turns: the chapter end, but never the end of the file (the last chapter of a
    // video ends with it, and a player at the end of the file no longer seeks back).
    readonly property real end: player.duration > 0 ? Math.min(to, player.duration - 250) : to
    readonly property var crop: Videos.crop(video)
    readonly property bool landscape: !compact && width > height
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

    onCurrentChanged: {
        shown(current.id)
        // After the bindings of the new entry (from, to): onCurrentChanged runs before them.
        if (player.source.toString() === Videos.source(video).toString() && !startPending) {
            Qt.callLater(page.restart)
        }
    }
    Component.onCompleted: shown(current.id)
    Component.onDestruction: player.stop()

    function restart() {
        player.position = from
        player.play()
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
            muted: player.playbackRate < 1
        }
        playbackRate: 1
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

            readonly property real ratio: page.crop.verhaeltnis || 16 / 9
            Layout.fillWidth: true
            Layout.fillHeight: page.landscape
            Layout.preferredHeight: page.landscape ? -1 : width / ratio

            Rectangle {
                anchors.fill: parent
                color: KanteStyle.stageColor
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
                color: KanteStyle.stageTextColor
                style: Text.Outline
                styleColor: KanteStyle.stageColor
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

                QQC2.Label {
                    Layout.fillWidth: true
                    visible: !page.landscape && !!page.current.attack
                    text: page.current.attack || ""
                    color: KanteStyle.disabledTextColor
                    wrapMode: Text.Wrap
                }

                QQC2.Slider {
                    id: timeline

                    Layout.fillWidth: true
                    from: page.from
                    to: page.to > page.from ? page.to : page.from + 1
                    value: player.position
                    onMoved: player.position = value
                    Accessible.name: qsTr("Position")

                    KanteSliderSkin { control: timeline }
                }

                KanteTransportBar {
                    id: transport

                    Layout.fillWidth: true
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

                RatingRow {
                    Layout.fillWidth: true
                    entry: page.current.id || ""
                    onRated: if (page.training) page.nextOpen()
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

                KanteHintCard {
                    objectName: "trainingDone"
                    Layout.fillWidth: true
                    visible: page.trainingDone
                    tier: KanteHintCard.Tier.Blue
                    titleText: qsTr("Done")
                    text: qsTr("Every technique of today's training is practised.")

                    KanteButton {
                        visible: !page.compact
                        text: qsTr("Back")
                        onClicked: page.backRequested()
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    visible: !page.landscape && !page.compact

                    KanteToolButton {
                        enabled: page.index > 0
                        icon.name: "media-skip-backward"
                        text: qsTr("Previous technique")
                        onClicked: page.go(-1)
                    }
                    Item {
                        Layout.fillWidth: true
                    }
                    KanteToolButton {
                        enabled: page.index < page.playlist.length - 1
                        icon.name: "media-skip-forward"
                        text: qsTr("Next technique")
                        onClicked: page.go(1)
                    }
                }

                KanteToolButton {
                    objectName: "backButton"
                    visible: page.landscape
                    icon.name: "go-previous-view"
                    text: qsTr("Back")
                    onClicked: page.backRequested()
                }
            }
        }
    }
}
