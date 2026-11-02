import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import Kante

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

    readonly property real base: KanteStyle.defaultFont.pointSize > 0 ? KanteStyle.defaultFont.pointSize : 10
    readonly property bool hasAttack: !techniqueOnly && !!point.attack
    // Kante's heading font in the sizes Kante itself uses: the technique as large as a KanteKpi
    // figure, the attack and the Japanese as a KantePageTitle.
    readonly property font techniqueFont: KanteStyle.headingFont(base * 2.6 * zoom)
    readonly property font attackFont: KanteStyle.headingFont(base * 1.35 * zoom)
    readonly property font kanjiFont: KanteStyle.japaneseFont(KanteStyle.titleFont(base * 1.35 * zoom))

    spacing: KanteStyle.unit(10) * zoom

    KanteSectionLabel {
        objectName: "boardGroup"
        Layout.alignment: Qt.AlignHCenter
        visible: !board.techniqueOnly && !!board.point.group
        text: board.point.group || ""
    }

    GridLayout {
        Layout.fillWidth: true
        columns: 2
        columnSpacing: KanteStyle.unit(12) * board.zoom
        rowSpacing: KanteStyle.unit(10) * board.zoom

        Rectangle {
            Layout.columnSpan: 2
            Layout.fillWidth: true
            implicitHeight: 1
            visible: board.hasAttack
            color: KanteStyle.ruleColor
        }
        KanteSectionLabel {
            Layout.alignment: Qt.AlignTop
            Layout.topMargin: KanteStyle.unit(6) * board.zoom
            visible: board.hasAttack
            text: qsTr("Attack")
        }
        ColumnLayout {
            Layout.fillWidth: true
            visible: board.hasAttack
            spacing: 0

            QQC2.Label {
                objectName: "boardAttack"
                Layout.fillWidth: true
                text: board.point.attack || ""
                font: board.attackFont
                wrapMode: Text.Wrap
            }
            QQC2.Label {
                Layout.fillWidth: true
                visible: !!board.point.attackExtra
                text: board.point.attackExtra || ""
                color: KanteStyle.mutedTextColor
                wrapMode: Text.Wrap
            }
            QQC2.Label {
                Layout.fillWidth: true
                visible: !!board.point.attackJp
                text: board.point.attackJp || ""
                color: KanteStyle.mutedTextColor
                font: board.kanjiFont
                wrapMode: Text.Wrap
            }
        }

        Rectangle {
            Layout.columnSpan: 2
            Layout.fillWidth: true
            implicitHeight: 1
            color: KanteStyle.ruleColor
        }
        KanteSectionLabel {
            Layout.alignment: Qt.AlignTop
            Layout.topMargin: KanteStyle.unit(8) * board.zoom
            text: qsTr("Technique")
        }
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 0

            QQC2.Label {
                objectName: "boardTechnique"
                Layout.fillWidth: true
                text: (board.point.nr ? board.point.nr + " · " : "") + (board.point.title || "")
                font: board.techniqueFont
                wrapMode: Text.Wrap
            }
            QQC2.Label {
                objectName: "boardExtra"
                Layout.fillWidth: true
                visible: !board.techniqueOnly && !!board.point.extra
                text: board.point.extra || ""
                color: KanteStyle.mutedTextColor
                wrapMode: Text.Wrap
            }
            QQC2.Label {
                objectName: "boardKanji"
                Layout.fillWidth: true
                visible: !!board.point.jp
                text: board.point.jp || ""
                color: KanteStyle.mutedTextColor
                font: board.kanjiFont
                wrapMode: Text.Wrap
            }
        }
    }
}
