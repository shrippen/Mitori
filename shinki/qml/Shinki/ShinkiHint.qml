import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts

// Hint (bible section 5, `.novid`): Info on `chip`, text inkMuted; Important on paperAlt with a
// 1 px frame and 3 px red on the left (`.novid.buy`); Error: the title in red, with its word.
// Actions (buttons) go in as children, after the text. `meta` a small line below the title (place,
// date); `compact` the tighter padding for a hint among other content.
Item {
    id: hint

    enum Kind {
        Info,
        Important,
        Error
    }
    property int kind: ShinkiHint.Kind.Info
    property string titleText: ""
    property string text: ""
    property string meta: ""
    property bool compact: false
    default property alias actions: actionRow.data

    readonly property int padding: compact ? ShinkiStyle.gridSpace * 2 : ShinkiStyle.cardSpace

    implicitWidth: column.implicitWidth + 2 * padding
    implicitHeight: column.implicitHeight + 2 * padding

    Rectangle {
        anchors.fill: parent
        color: hint.kind === ShinkiHint.Kind.Info ? ShinkiStyle.chip : ShinkiStyle.paperAlt
        border.width: hint.kind === ShinkiHint.Kind.Info ? 0 : ShinkiStyle.lineWidth
        border.color: ShinkiStyle.line
    }
    Rectangle {
        visible: hint.kind !== ShinkiHint.Kind.Info
        width: ShinkiStyle.selectedBarWidth
        height: parent.height
        color: ShinkiStyle.red
    }

    ColumnLayout {
        id: column

        anchors.fill: parent
        anchors.margins: hint.padding
        spacing: ShinkiStyle.gridSpace

        QQC2.Label {
            Layout.fillWidth: true
            visible: hint.titleText !== ""
            text: hint.titleText
            color: hint.kind === ShinkiHint.Kind.Error ? ShinkiStyle.red : ShinkiStyle.ink
            font.family: ShinkiStyle.uiFamily
            font.bold: true
            font.pixelSize: ShinkiStyle.uiSize
            wrapMode: Text.Wrap
        }
        QQC2.Label {
            Layout.fillWidth: true
            visible: hint.meta !== ""
            text: hint.meta
            color: ShinkiStyle.inkMuted
            font.family: ShinkiStyle.uiFamily
            font.pixelSize: ShinkiStyle.smallSize
            wrapMode: Text.Wrap
        }
        QQC2.Label {
            Layout.fillWidth: true
            visible: hint.text !== ""
            text: hint.text
            color: ShinkiStyle.inkMuted
            font.family: ShinkiStyle.uiFamily
            font.pixelSize: ShinkiStyle.hintSize
            wrapMode: Text.Wrap
        }
        // The actions read as links: red and bold (`.novid b`).
        RowLayout {
            id: actionRow

            visible: children.length > 0
            spacing: ShinkiStyle.gridSpace * 2
            onChildrenChanged: {
                for (const child of children) {
                    if (child.link !== undefined) {
                        child.link = true
                    }
                }
            }
        }
    }
}
