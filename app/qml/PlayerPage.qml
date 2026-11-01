import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import QtMultimedia
import org.kde.kirigami as Kirigami
import Kante
import Mitori

// Player (clickdummy "Player" and "Querformat"): the technique's chapter of the exam video,
// without the black bars, in a loop by default; four tempi (sound off below 1×), single frames,
// double tap left/right = 2 s back/on. Landscape: the controls in a column on the right.
// Brightness and volume gestures come with the platform layer (ROADMAP.md).
Kirigami.Page {
    id: page

    required property var playlist
    required property int index
    readonly property var current: playlist[index] || ({})
    readonly property string video: current.video ? current.video.video : ""
    readonly property real from: current.video ? current.video.von * 1000 : 0
    readonly property real to: current.video ? current.video.bis * 1000 : 0
    // Where the loop turns: the chapter end, but never the end of the file (the last chapter of a
    // video ends with it, and a player at the end of the file no longer seeks back).
    readonly property real end: player.duration > 0 ? Math.min(to, player.duration - 250) : to
    readonly property var crop: Videos.crop(video)
    readonly property bool landscape: width > height
    readonly property var tempi: [0.25, 0.5, 0.75, 1]
    readonly property int frameMs: 40       // 25 frames per second (PAL DVDs)
    readonly property int jumpMs: 2000      // double tap
    property bool looping: true
    // For the UI test: where the video stands (ms) and whether it plays.
    readonly property alias position: player.position
    readonly property alias playing: player.playing
    readonly property alias duration: player.duration

    // The entry on screen, for the programme list to mark it when the user comes back.
    signal shown(string id)

    title: current.title || ""
    padding: 0
    globalToolBarStyle: landscape ? Kirigami.ApplicationHeaderStyle.None : Kirigami.ApplicationHeaderStyle.Auto

    onCurrentChanged: {
        shown(current.id)
        if (player.source.toString() === Videos.source(video).toString() && player.mediaStatus !== MediaPlayer.LoadingMedia) {
            page.restart()
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
        onMediaStatusChanged: {
            if (player.mediaStatus === MediaPlayer.LoadedMedia) {
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

        ColumnLayout {
            Layout.fillWidth: !page.landscape
            Layout.fillHeight: page.landscape
            Layout.preferredWidth: page.landscape ? Kirigami.Units.gridUnit * 14 : -1
            Layout.margins: Kirigami.Units.largeSpacing
            spacing: Kirigami.Units.largeSpacing

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

            QQC2.Label {
                Layout.fillWidth: true
                text: page.timeText(player.position - page.from) + " / " + page.timeText(page.to - page.from)
                font.family: KanteStyle.monoFamily
                horizontalAlignment: Text.AlignHCenter
            }

            Flow {
                Layout.fillWidth: true
                spacing: Kirigami.Units.smallSpacing

                KanteToolButton {
                    visible: !page.landscape
                    enabled: page.index > 0
                    icon.name: "media-skip-backward"
                    text: qsTr("Previous technique")
                    display: QQC2.AbstractButton.IconOnly
                    onClicked: page.go(-1)
                }
                KanteToolButton {
                    icon.name: "go-previous"
                    text: qsTr("Frame back")
                    display: QQC2.AbstractButton.IconOnly
                    onClicked: page.step(-1)
                }
                KanteToolButton {
                    icon.name: player.playing ? "media-playback-pause" : "media-playback-start"
                    text: player.playing ? qsTr("Pause") : qsTr("Play")
                    display: QQC2.AbstractButton.IconOnly
                    onClicked: page.togglePlay()
                }
                KanteToolButton {
                    icon.name: "go-next"
                    text: qsTr("Frame on")
                    display: QQC2.AbstractButton.IconOnly
                    onClicked: page.step(1)
                }
                KanteToolButton {
                    visible: !page.landscape
                    enabled: page.index < page.playlist.length - 1
                    icon.name: "media-skip-forward"
                    text: qsTr("Next technique")
                    display: QQC2.AbstractButton.IconOnly
                    onClicked: page.go(1)
                }
                KanteToolButton {
                    checkable: true
                    checked: page.looping
                    icon.name: "media-playlist-repeat"
                    text: qsTr("Loop")
                    display: QQC2.AbstractButton.IconOnly
                    onToggled: page.looping = checked
                }
            }

            KanteSegmented {
                Layout.fillWidth: true
                model: page.tempi.map(t => t.toLocaleString(Qt.locale(), "f", 2).replace(/0+$/, "").replace(/[.,]$/, "") + "×")
                currentIndex: page.tempi.indexOf(player.playbackRate)
                onActivated: index => player.playbackRate = page.tempi[index]
                Accessible.name: qsTr("Tempo")
            }

            KanteToolButton {
                visible: page.landscape
                icon.name: "go-previous-view"
                text: qsTr("Back")
                onClicked: page.Kirigami.PageStack.pop()
            }

            Item {
                Layout.fillHeight: true
            }
        }
    }
}
