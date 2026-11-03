pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts

// Segments (bible section 5, `.seg`): equal fields in a row, 1 px frame, the text bold in
// inkMuted; the chosen one filled red with white text. Left and Right move the choice.
//   model          the labels
//   currentIndex   the chosen one
//   activated(i)   a field was chosen
FocusScope {
    id: seg

    property var model: []
    property int currentIndex: 0
    signal activated(int index)

    // Equal fields, each as wide as the longest label needs (a segmented control beside a
    // setting's title takes that width, not the row's).
    implicitHeight: ShinkiStyle.rowMinSpace
    implicitWidth: seg.model.length * (widest.advanceWidth + 4 * ShinkiStyle.gridSpace) + 2 * ShinkiStyle.lineWidth
    activeFocusOnTab: true

    Accessible.role: Accessible.PageTabList

    Keys.onLeftPressed: seg.choose(seg.currentIndex - 1)
    Keys.onRightPressed: seg.choose(seg.currentIndex + 1)

    function choose(index) {
        if (index < 0 || index >= seg.model.length) {
            return
        }
        seg.activated(index)
    }

    TextMetrics {
        id: widest

        font.family: ShinkiStyle.uiFamily
        font.bold: true
        font.pixelSize: ShinkiStyle.segmentSize
        text: seg.model.reduce((a, b) => String(b).length > a.length ? String(b) : a, "")
    }

    Rectangle {
        anchors.fill: parent
        color: ShinkiStyle.paper
        border.width: seg.activeFocus ? ShinkiStyle.focusWidth : ShinkiStyle.lineWidth
        border.color: seg.activeFocus ? ShinkiStyle.ink : ShinkiStyle.line
    }

    RowLayout {
        id: row

        anchors.fill: parent
        anchors.margins: ShinkiStyle.lineWidth
        spacing: 0

        Repeater {
            model: seg.model

            QQC2.AbstractButton {
                id: field

                required property int index
                required property var modelData
                readonly property bool current: seg.currentIndex === index

                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredWidth: 1
                focusPolicy: Qt.NoFocus
                text: String(modelData)
                Accessible.role: Accessible.PageTab
                Accessible.checked: current
                onClicked: seg.activated(index)

                background: Rectangle {
                    color: field.current ? ShinkiStyle.red : field.hovered ? ShinkiStyle.chip : "transparent"

                    Behavior on color {
                        ColorAnimation { duration: ShinkiStyle.stateDuration }
                    }
                }
                contentItem: QQC2.Label {
                    text: field.text
                    color: field.current ? ShinkiStyle.textOnRed : ShinkiStyle.inkMuted
                    font.family: ShinkiStyle.uiFamily
                    font.bold: true
                    font.pixelSize: ShinkiStyle.segmentSize
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideRight
                }
            }
        }
    }
}
