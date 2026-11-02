import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import Kante
import Mitori

// An exam round (clickdummy "Prüfungsrunde"): one point at a time on the board, samples from
// earlier grades marked. "Next" (Space, Enter) moves on; after the last the round is rated
// (ExamEndPage). No video, no time limit. Cancel goes back to the settings.
Kirigami.Page {
    id: page

    // The page keeps the platform's colours (Theme.inherit: false): give it Kante's, and
    // Kante's page ground (the scope's background role is the dialog colour).
    property Item kanteScope: KanteScope { target: page }
    background: Rectangle { color: KanteStyle.backgroundColor }

    required property var points        // Exam.draw()
    required property string discipline
    required property string grade
    property int index: 0
    readonly property var current: points[index] || ({})
    // A tablet or a phone held sideways: the board grows with the room.
    readonly property bool wide: width > height
    readonly property bool tablet: Math.min(width, height) >= Kirigami.Units.gridUnit * 26

    title: qsTr("Mock exam")

    actions: [
        Kirigami.Action {
            icon.name: "dialog-cancel"
            text: qsTr("Cancel")
            onTriggered: page.Kirigami.PageStack.pop()
        }
    ]

    function next() {
        if (page.index < page.points.length - 1) {
            page.index++
            return
        }
        const window = page.QQC2.ApplicationWindow.window as Kirigami.ApplicationWindow
        window.pageStack.replace(Qt.resolvedUrl("ExamEndPage.qml"),
                                 { points: page.points, discipline: page.discipline, grade: page.grade })
    }

    focus: true
    Keys.onSpacePressed: page.next()
    Keys.onReturnPressed: page.next()
    Keys.onEnterPressed: page.next()
    Component.onCompleted: forceActiveFocus()

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Kirigami.Units.largeSpacing
        spacing: Kirigami.Units.largeSpacing

        QQC2.Label {
            objectName: "counter"
            Layout.alignment: Qt.AlignRight
            text: (page.index + 1) + " / " + page.points.length
            color: KanteStyle.mutedTextColor
            font: KanteStyle.monoFont(0, false)
            Accessible.name: qsTr("Point %1 of %2").arg(page.index + 1).arg(page.points.length)
        }

        // The board in the middle of the free room; it scrolls if a long name does not fit.
        QQC2.ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            contentWidth: availableWidth

            Item {
                width: parent.width
                implicitHeight: Math.max(column.implicitHeight, page.height * 0.5)

                ColumnLayout {
                    id: column

                    anchors.centerIn: parent
                    width: Math.min(parent.width, Kirigami.Units.gridUnit * (page.tablet ? 40 : 28))
                    spacing: Kirigami.Units.largeSpacing

                    ExamBoard {
                        objectName: "examBoard"
                        Layout.fillWidth: true
                        point: page.current
                        techniqueOnly: page.discipline === "shinkitoho"
                        zoom: page.tablet ? 1.6 : 1
                    }
                    KanteChip {
                        objectName: "sampleMark"
                        Layout.alignment: Qt.AlignHCenter
                        visible: !!page.current.sample
                        interactive: false
                        text: qsTr("Sample %1").arg(page.current.gradeName || "")
                    }
                }
            }
        }

        KanteButton {
            objectName: "nextButton"
            Layout.alignment: Qt.AlignHCenter
            Layout.preferredWidth: Math.min(parent.width, Kirigami.Units.gridUnit * 20)
            emphasis: KanteButton.Emphasis.Primary
            text: page.index < page.points.length - 1 ? qsTr("Next") : qsTr("Rate the round")
            onClicked: page.next()
        }
        QQC2.Label {
            Layout.fillWidth: true
            visible: !page.wide || page.tablet
            horizontalAlignment: Text.AlignHCenter
            text: qsTr("You rate every technique at the end of the round.")
            color: KanteStyle.mutedTextColor
            font: KanteStyle.smallFont
            wrapMode: Text.Wrap
        }
    }
}
