import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts

// Toast (bible section 5): ink at 92 % as the ground, white text 12 px, square; a warning or an
// error starts with 3 px red at the left (the word says what happened, red does not stand
// alone). `show()` shows it for `life` ms (0: stays), then `closed()`. `actionText` puts a
// link-like button at the end (`actionTriggered()`). Same interface as KanteToast (kind Info, Ok,
// Warn, Danger; title, text, life, shown).
Item {
    id: toast

    enum Kind {
        Info,
        Ok,
        Warn,
        Danger
    }
    property int kind: ShinkiToast.Kind.Info
    property string title: ""
    property string text: ""
    property int life: 4000
    property bool shown: false
    property string actionText: ""
    property string actionShortcut: ""
    signal closed()
    signal actionTriggered()

    function show() {
        shown = true
        if (life > 0) {
            timer.restart()
        }
    }
    function hide() {
        if (!shown) {
            return
        }
        shown = false
        timer.stop()
        closed()
    }

    implicitWidth: Math.min(ShinkiStyle.dialogSmallSize, row.implicitWidth + 2 * ShinkiStyle.cardSpace)
    implicitHeight: row.implicitHeight + 2 * ShinkiStyle.cardSpace
    width: implicitWidth
    height: implicitHeight
    visible: opacity > 0
    opacity: shown ? 1 : 0
    z: 1000

    Behavior on opacity {
        NumberAnimation { duration: ShinkiStyle.stateDuration }
    }

    Accessible.role: Accessible.AlertMessage
    Accessible.name: title !== "" ? title + ": " + text : text

    // Under the pointer it stays, then gets its full life again.
    HoverHandler {
        id: hold

        onHoveredChanged: {
            if (hovered) {
                timer.stop()
            } else if (toast.shown && toast.life > 0) {
                timer.restart()
            }
        }
    }
    Timer {
        id: timer

        interval: toast.life
        onTriggered: toast.hide()
    }

    Rectangle {
        anchors.fill: parent
        color: Qt.alpha(ShinkiStyle.ink, ShinkiStyle.tooltipOpacity)
    }
    Rectangle {
        visible: toast.kind === ShinkiToast.Kind.Warn || toast.kind === ShinkiToast.Kind.Danger
        width: ShinkiStyle.selectedBarWidth
        height: parent.height
        color: ShinkiStyle.red
    }

    RowLayout {
        id: row

        anchors.fill: parent
        anchors.margins: ShinkiStyle.cardSpace
        spacing: ShinkiStyle.cardSpace

        ColumnLayout {
            Layout.fillWidth: true
            spacing: ShinkiStyle.gridSpace

            QQC2.Label {
                Layout.fillWidth: true
                visible: toast.title !== ""
                text: toast.title
                color: ShinkiStyle.textOnRed
                font.family: ShinkiStyle.uiFamily
                font.bold: true
                font.pixelSize: ShinkiStyle.smallSize
                wrapMode: Text.Wrap
            }
            QQC2.Label {
                Layout.fillWidth: true
                Layout.maximumWidth: ShinkiStyle.dialogSmallSize - 2 * ShinkiStyle.cardSpace
                visible: toast.text !== ""
                text: toast.text
                color: ShinkiStyle.textOnRed
                font.family: ShinkiStyle.uiFamily
                font.pixelSize: ShinkiStyle.smallSize
                wrapMode: Text.Wrap
            }
        }
        QQC2.AbstractButton {
            visible: toast.actionText !== ""
            text: toast.actionText
            onClicked: {
                toast.actionTriggered()
                toast.hide()
            }
            contentItem: QQC2.Label {
                text: parent.text
                color: ShinkiStyle.textOnRed
                font.family: ShinkiStyle.uiFamily
                font.bold: true
                font.pixelSize: ShinkiStyle.smallSize
                font.underline: true
            }
        }
    }
}
