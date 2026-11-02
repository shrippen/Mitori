import QtQuick
import QtQuick.Controls as QQC2
import Kante

// A place on the technique map: filled in its level's colour (KanteStyle.levelColor, as the
// programme and the statistics), `label` written on it. Hovering or keyboard focus shows `tip`;
// a tap, Enter or Space activates it, the arrow keys ask to move on (`moved`).
Item {
    id: place

    property int level: 0
    property string label: ""
    property string tip: ""
    // Levels are shades of the focus colour over the sunken ground (as KanteCalendarHeatmap).
    readonly property color fill: level > 0 ? KanteStyle.levelColor(level) : "transparent"

    signal activated()
    signal moved(int down, int across)

    Accessible.role: Accessible.Button
    Accessible.name: tip
    Accessible.onPressAction: place.activated()

    Rectangle {
        anchors.fill: parent
        color: KanteStyle.sunkenColor
    }
    Rectangle {
        anchors.fill: parent
        color: place.fill
    }
    Text {
        anchors.centerIn: parent
        text: place.label
        font.family: KanteStyle.smallFont.family
        font.pointSize: KanteStyle.smallFont.pointSize
        font.bold: true
        // The strong shades carry the ground's ink, the light ones the text colour.
        color: place.level >= 3 ? KanteStyle.inkOn(KanteStyle.focusColor) : KanteStyle.strongTextColor
    }
    // Not the focus colour: the strongest level is filled with it.
    Rectangle {
        anchors.fill: parent
        visible: place.activeFocus
        color: "transparent"
        border.width: 2
        border.color: KanteStyle.strongTextColor
    }

    HoverHandler {
        id: hover

        cursorShape: Qt.PointingHandCursor
    }
    TapHandler {
        onTapped: place.activated()
    }
    QQC2.ToolTip.text: tip
    QQC2.ToolTip.visible: hover.hovered || place.activeFocus
    QQC2.ToolTip.delay: place.activeFocus ? 0 : 300

    Keys.onPressed: event => {
        const moves = {
            [Qt.Key_Left]: [0, -1],
            [Qt.Key_Right]: [0, 1],
            [Qt.Key_Up]: [-1, 0],
            [Qt.Key_Down]: [1, 0]
        }
        if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter || event.key === Qt.Key_Space) {
            place.activated()
            event.accepted = true
            return
        }
        const move = moves[event.key]
        if (!move) {
            return
        }
        place.moved(move[0], move[1])
        event.accepted = true
    }
}
