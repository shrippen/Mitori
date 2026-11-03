import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

// The description of a sequence (the Kamae exercise of Shinki Toho): its positions in order,
// numbered, with their kanji. Seigan comes twice (KORREKTUREN S-09). Hidden without steps.
//
//   POSITIONS IN ONE FLOW
//    1  Rei                 礼
//    2  Battô             抜刀
ColumnLayout {
    id: list

    // [{title, jp}] as Programme gives an entry's `steps`.
    property var steps: []
    readonly property int count: repeater.count

    objectName: "stepList"
    visible: steps.length > 0
    spacing: Kirigami.Units.smallSpacing

    LookSectionLabel {
        Layout.fillWidth: true
        rule: true
        text: qsTr("Positions in one flow")
    }
    Repeater {
        id: repeater

        model: list.steps

        RowLayout {
            id: step

            required property var modelData
            required property int index

            objectName: "step"
            Layout.fillWidth: true
            spacing: Kirigami.Units.smallSpacing

            QQC2.Label {
                Layout.minimumWidth: Kirigami.Units.gridUnit * 1.5
                horizontalAlignment: Text.AlignRight
                text: step.index + 1
                font: Look.monoFont(Kirigami.Theme.smallFont.pointSize)
                color: Look.mutedTextColor
            }
            QQC2.Label {
                Layout.fillWidth: true
                text: step.modelData.title
                wrapMode: Text.Wrap
            }
            QQC2.Label {
                text: step.modelData.jp
                font: Look.japaneseFont()
                color: Look.mutedTextColor
            }
        }
    }
}
