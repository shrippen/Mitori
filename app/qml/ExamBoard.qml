import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import Mitori

// The board of an exam round (app-entscheidungen.md, Runde 4: design D): the group (standing,
// sitting …), below it "Attack" and "Technique" labelled in two rows, each with its addition and
// the Japanese. Shinki Toho shows only the technique: number, name and kanji; its addition
// (In · Seigan → Hidari-gote) would give the answer away.
ColumnLayout {
    id: board

    required property var point
    property bool techniqueOnly: false
    // Larger on a tablet or in landscape.
    property real zoom: 1

    readonly property real base: Look.defaultFont.pointSize > 0 ? Look.defaultFont.pointSize : 10
    readonly property bool hasAttack: !techniqueOnly && !!point.attack
    // The style's heading font in Kante's sizes: the technique as large as a Kpi figure, the
    // attack and the Japanese as a page title.
    readonly property font techniqueFont: Look.headingFont(base * 2.6 * zoom)
    readonly property font attackFont: Look.headingFont(base * 1.35 * zoom)
    readonly property font kanjiFont: Look.japaneseFont(Look.titleFont(base * 1.35 * zoom))

    spacing: Look.unit(10) * zoom

    LookSectionLabel {
        objectName: "boardGroup"
        Layout.alignment: Qt.AlignHCenter
        visible: !board.techniqueOnly && !!board.point.group
        text: board.point.group || ""
    }

    GridLayout {
        Layout.fillWidth: true
        columns: 2
        columnSpacing: Look.unit(12) * board.zoom
        rowSpacing: Look.unit(10) * board.zoom

        Rectangle {
            Layout.columnSpan: 2
            Layout.fillWidth: true
            implicitHeight: 1
            visible: board.hasAttack
            color: Look.ruleColor
        }
        LookSectionLabel {
            Layout.alignment: Qt.AlignTop
            Layout.topMargin: Look.unit(6) * board.zoom
            visible: board.hasAttack
            text: qsTr("Attack")
        }
        ColumnLayout {
            Layout.fillWidth: true
            visible: board.hasAttack
            spacing: 0

            LookLabel {
                objectName: "boardAttack"
                Layout.fillWidth: true
                text: board.point.attack || ""
                font: board.attackFont
                wrapMode: Text.Wrap
            }
            LookLabel {
                Layout.fillWidth: true
                visible: !!board.point.attackExtra
                text: board.point.attackExtra || ""
                color: Look.mutedTextColor
                wrapMode: Text.Wrap
            }
            LookLabel {
                Layout.fillWidth: true
                visible: !!board.point.attackJp
                text: board.point.attackJp || ""
                color: Look.mutedTextColor
                font: board.kanjiFont
                wrapMode: Text.Wrap
            }
        }

        Rectangle {
            Layout.columnSpan: 2
            Layout.fillWidth: true
            implicitHeight: 1
            color: Look.ruleColor
        }
        LookSectionLabel {
            Layout.alignment: Qt.AlignTop
            Layout.topMargin: Look.unit(8) * board.zoom
            text: qsTr("Technique")
        }
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 0

            LookLabel {
                objectName: "boardTechnique"
                Layout.fillWidth: true
                text: (board.point.nr ? board.point.nr + " · " : "") + (board.point.title || "")
                font: board.techniqueFont
                wrapMode: Text.Wrap
            }
            LookLabel {
                objectName: "boardExtra"
                Layout.fillWidth: true
                visible: !board.techniqueOnly && !!board.point.extra
                text: board.point.extra || ""
                color: Look.mutedTextColor
                wrapMode: Text.Wrap
            }
            LookLabel {
                objectName: "boardKanji"
                Layout.fillWidth: true
                visible: !!board.point.jp
                text: board.point.jp || ""
                color: Look.mutedTextColor
                font: board.kanjiFont
                wrapMode: Text.Wrap
            }
        }
    }
}
