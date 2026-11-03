import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts

// Option tile (bible section 5, Optionskachel): one of several to choose from, as the tile of
// section 5 (1 px frame, white): an icon or glyph, the name 16 px bold, a hint 12,5 px inkMuted.
// Chosen: the `selected` ground, a red frame and 3 px red at the bottom (not colour alone).
// Keyboard focus: 2 px ink. Same interface as KanteOptionTile (icon, glyph, text, hint, checked,
// checkable, autoExclusive).
QQC2.AbstractButton {
    id: tile

    property string hint: ""
    property string glyph: ""

    checkable: true
    autoExclusive: true
    hoverEnabled: true
    padding: ShinkiStyle.cardSpace
    implicitWidth: ShinkiStyle.unit(140)
    implicitHeight: Math.max(ShinkiStyle.rowMinSpace, implicitContentHeight + topPadding + bottomPadding)

    Accessible.role: checkable ? Accessible.RadioButton : Accessible.Button
    Accessible.name: text
    Accessible.description: hint

    Keys.onReturnPressed: tile.clicked()
    Keys.onEnterPressed: tile.clicked()

    background: Rectangle {
        color: tile.checked ? ShinkiStyle.selected : tile.hovered ? ShinkiStyle.paperAlt : ShinkiStyle.paper
        border.width: tile.visualFocus ? ShinkiStyle.focusWidth : ShinkiStyle.lineWidth
        border.color: tile.visualFocus ? ShinkiStyle.ink : tile.checked || tile.hovered ? ShinkiStyle.red : ShinkiStyle.line

        Rectangle {
            visible: tile.checked
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.margins: ShinkiStyle.lineWidth
            height: ShinkiStyle.selectedBarWidth
            color: ShinkiStyle.red
        }
    }

    contentItem: RowLayout {
        spacing: ShinkiStyle.cardSpace

        QQC2.Label {
            visible: tile.glyph !== ""
            text: tile.glyph
            color: ShinkiStyle.red
            font.family: ShinkiStyle.japaneseFamily
            font.pixelSize: ShinkiStyle.tileKanjiSize
        }
        ShinkiIcon {
            visible: tile.glyph === "" && tile.icon.name !== ""
            name: tile.icon.name
            color: tile.checked ? ShinkiStyle.red : ShinkiStyle.ink
        }
        ColumnLayout {
            Layout.fillWidth: true
            spacing: ShinkiStyle.gridSpace

            QQC2.Label {
                Layout.fillWidth: true
                text: tile.text
                color: ShinkiStyle.ink
                font.family: ShinkiStyle.uiFamily
                font.bold: true
                font.pixelSize: ShinkiStyle.tileTitleSize
                wrapMode: Text.Wrap
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
