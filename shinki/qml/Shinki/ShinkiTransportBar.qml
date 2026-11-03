import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts

// Transport (bible section 5, "Player", `.ctrl` and `.speeds`): back to the start, a frame back,
// play as a red circle, a frame on, the loop; under it the tempi, the chosen one filled in ink
// (so it does not compete with the red). The state comes in through properties, every action goes
// out as a signal; the keys are those of KanteTransportBar (K, L, Home, comma, period, +, −).
FocusScope {
    id: bar

    property bool playing: false
    property bool looping: false
    property real speed: 1.0
    property var speedSteps: []
    property bool frameStep: false
    property bool metronomeVisible: false
    property bool countInVisible: false
    property string positionText: ""
    property string rewindText: "Zum Anfang"
    property string playText: "Abspielen"
    property string pauseText: "Pause"
    property string loopText: "Schleife"
    property string speedText: "Tempo"
    property string frameBackText: "Bild zurück"
    property string frameOnText: "Bild vor"
    property string positionName: "Position"
    property string accessibleName: "Wiedergabe"

    signal playToggled()
    signal loopToggled()
    signal speedChangeRequested(real speed)
    signal rewind()
    signal frameStepRequested(int direction)

    // The next tempo above (+) or below (−).
    function stepSpeed(dir) {
        const next = speedSteps.filter(s => dir > 0 ? s > speed : s < speed)
        if (next.length === 0) {
            return
        }
        speedChangeRequested(dir > 0 ? Math.min(...next) : Math.max(...next))
    }
    function speedLabel(step) {
        return String(step).replace(".", Qt.locale().decimalPoint) + (step === 1 ? "×" : "")
    }

    implicitWidth: column.implicitWidth
    implicitHeight: column.implicitHeight
    activeFocusOnTab: false

    Accessible.role: Accessible.ToolBar
    Accessible.name: accessibleName

    Keys.onPressed: event => {
        switch (event.key) {
        case Qt.Key_K: playToggled(); break
        case Qt.Key_L: loopToggled(); break
        case Qt.Key_Comma: if (!frameStep) return; frameStepRequested(-1); break
        case Qt.Key_Period: if (!frameStep) return; frameStepRequested(1); break
        case Qt.Key_Home: rewind(); break
        case Qt.Key_Plus:
        case Qt.Key_Equal: stepSpeed(1); break
        case Qt.Key_Minus: stepSpeed(-1); break
        default: return
        }
        event.accepted = true
    }

    ColumnLayout {
        id: column

        anchors.fill: parent
        spacing: ShinkiStyle.gridSpace

        RowLayout {
            Layout.alignment: Qt.AlignHCenter
            spacing: ShinkiStyle.gridSpace * 2

            ShinkiToolButton {
                icon.name: "media-skip-backward"
                text: bar.rewindText
                display: QQC2.AbstractButton.IconOnly
                onClicked: bar.rewind()
            }
            ShinkiToolButton {
                visible: bar.frameStep
                icon.name: "go-previous"
                text: bar.frameBackText
                display: QQC2.AbstractButton.IconOnly
                onClicked: bar.frameStepRequested(-1)
            }
            // Play: the one round red control (bible section 4).
            QQC2.AbstractButton {
                id: play

                objectName: "shinkiPlay"
                implicitWidth: ShinkiStyle.playLargeSize
                implicitHeight: implicitWidth
                text: bar.playing ? bar.pauseText : bar.playText
                Accessible.name: text
                onClicked: bar.playToggled()

                background: Rectangle {
                    radius: width / 2
                    color: ShinkiStyle.red
                    border.width: play.visualFocus ? ShinkiStyle.focusWidth : 0
                    border.color: ShinkiStyle.ink
                }
                contentItem: Item {
                    // The icon as a mask, white on the red circle.
                    ShinkiIcon {
                        anchors.centerIn: parent
                        name: bar.playing ? "media-playback-pause" : "media-playback-start"
                        color: ShinkiStyle.textOnRed
                    }
                }
            }
            ShinkiToolButton {
                visible: bar.frameStep
                icon.name: "go-next"
                text: bar.frameOnText
                display: QQC2.AbstractButton.IconOnly
                onClicked: bar.frameStepRequested(1)
            }
            ShinkiToolButton {
                icon.name: "media-playlist-repeat"
                text: bar.loopText
                display: QQC2.AbstractButton.IconOnly
                checkable: true
                checked: bar.looping
                onClicked: {
                    checked = Qt.binding(() => bar.looping)
                    bar.loopToggled()
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: ShinkiStyle.gridSpace * 2

            QQC2.Label {
                text: bar.speedText
                color: ShinkiStyle.inkMuted
                font.family: ShinkiStyle.narrowFamily
                font.bold: true
                font.capitalization: Font.AllUppercase
                font.pixelSize: ShinkiStyle.columnSize
                font.letterSpacing: ShinkiStyle.columnSize * ShinkiStyle.columnTracking
            }
            Repeater {
                model: bar.speedSteps

                QQC2.AbstractButton {
                    id: tempo

                    required property var modelData
                    readonly property bool current: Math.abs(bar.speed - modelData) < 0.001

                    Layout.fillWidth: true
                    Layout.preferredWidth: 1        // equal widths (`.speeds span{flex:1}`)
                    implicitHeight: ShinkiStyle.chipMinSpace
                    focusPolicy: Qt.NoFocus
                    text: bar.speedLabel(modelData)
                    Accessible.checked: current
                    onClicked: bar.speedChangeRequested(modelData)

                    background: Rectangle {
                        color: tempo.current ? ShinkiStyle.ink : ShinkiStyle.paper
                        border.width: ShinkiStyle.lineWidth
                        border.color: tempo.current ? ShinkiStyle.ink : ShinkiStyle.line
                    }
                    contentItem: QQC2.Label {
                        text: tempo.text
                        color: tempo.current ? ShinkiStyle.textOnRed : ShinkiStyle.ink
                        font.family: ShinkiStyle.uiFamily
                        font.bold: tempo.current
                        font.pixelSize: ShinkiStyle.segmentSize
                        font.features: { "tnum": 1 }
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }
                }
            }
            QQC2.Label {
                text: bar.positionText
                color: ShinkiStyle.inkMuted
                font.family: ShinkiStyle.uiFamily
                font.pixelSize: ShinkiStyle.smallSize
                font.features: { "tnum": 1 }
                Accessible.name: bar.positionName + " " + text
            }
        }
    }
}
