import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import Kante
import Mitori

// Start menu (clickdummy "Startmenü", variant 1): tiles; the programme tile shows the discipline
// and grade chosen last. More tiles come with their features (ROADMAP.md).
Kirigami.ScrollablePage {
    id: page

    title: "Mitori 見取り"

    actions: [
        Kirigami.Action {
            icon.name: "configure"
            text: qsTr("Settings")
            displayHint: Kirigami.DisplayHint.IconOnly
            onTriggered: page.Kirigami.PageStack.push(Qt.resolvedUrl("SettingsPage.qml"))
        }
    ]

    ColumnLayout {
        spacing: Kirigami.Units.largeSpacing

        Kirigami.PlaceholderMessage {
            Layout.fillWidth: true
            visible: Programme.errorString !== ""
            icon.name: "data-error"
            text: qsTr("Programme data missing")
            explanation: Programme.errorString
        }

        KanteOptionTile {
            id: programmeTile

            readonly property var current: page.disciplineInfo(Programme.discipline)

            Layout.fillWidth: true
            visible: Programme.errorString === ""
            checkable: false
            autoExclusive: false
            glyph: "技"
            text: qsTr("Programme")
            hint: current.title + " · " + current.grade
            onClicked: page.Kirigami.PageStack.push(Qt.resolvedUrl("ProgrammePage.qml"))
        }
    }

    // Title of a discipline and the name of its chosen grade.
    function disciplineInfo(id) {
        const d = Programme.disciplines.find(x => x.id === id)
        if (!d) {
            return { title: "", grade: "" }
        }
        const gradeId = Programme.grade(id)
        const g = d.grade.find(x => x.id === gradeId)
        return { title: d.titel, grade: g ? g.name : "" }
    }
}
