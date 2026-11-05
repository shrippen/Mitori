import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts

// Tile (bible section 5, `.tile`): 1 px frame on paper, the kanji in red, the title bold, a line
// under it in inkMuted. `big`: the wide programme tile on paperAlt with a larger kanji. Under
// the pointer and with the keyboard focus the frame turns red (focus: 2 px, ink).
QQC2.AbstractButton {
    id: tile

    property string glyph: ""
    property string hint: ""
    property bool big: false

    hoverEnabled: true
    focusPolicy: Qt.StrongFocus
    padding: ShinkiStyle.cardSpace
    implicitWidth: implicitContentWidth + leftPadding + rightPadding
    implicitHeight: Math.max(ShinkiStyle.rowMinSpace, implicitContentHeight + topPadding + bottomPadding)

    Accessible.role: Accessible.Button
    Accessible.name: text
    Accessible.description: hint

    background: Rectangle {
        color: tile.big ? ShinkiStyle.paperAlt : ShinkiStyle.paper
        border.width: tile.visualFocus ? ShinkiStyle.focusWidth : ShinkiStyle.lineWidth
        border.color: tile.visualFocus ? ShinkiStyle.ink : tile.hovered || tile.down ? ShinkiStyle.red : ShinkiStyle.line

        Behavior on border.color {
            ColorAnimation { duration: ShinkiStyle.stateDuration }
        }
    }

    contentItem: RowLayout {
        spacing: ShinkiStyle.cardSpace

        QQC2.Label {
            Layout.alignment: Qt.AlignTop
            visible: tile.glyph !== ""
            text: tile.glyph
            color: ShinkiStyle.red
            font.family: ShinkiStyle.japaneseFamily
            font.pixelSize: tile.big ? ShinkiStyle.tileKanjiBigSize : ShinkiStyle.tileKanjiSize
            Accessible.ignored: true
        }
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 0

            QQC2.Label {
                Layout.fillWidth: true
                text: tile.text
                color: ShinkiStyle.ink
                font.family: ShinkiStyle.uiFamily
                font.bold: true
                font.pixelSize: tile.big ? ShinkiStyle.tileTitleBigSize : ShinkiStyle.tileTitleSize
                elide: Text.ElideRight
            }
            QQC2.Label {
                Layout.fillWidth: true
                visible: tile.hint !== ""
                text: tile.hint
                color: ShinkiStyle.inkMuted
                font.family: ShinkiStyle.uiFamily
                font.pixelSize: ShinkiStyle.hintSize
                wrapMode: Text.Wrap
            }
        }
    }
}
