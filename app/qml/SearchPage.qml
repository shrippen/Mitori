pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import Kante
import Mitori

// Search (clickdummy "Glossar & Suche", the search part): techniques of every discipline and
// grade by name, attack or Japanese, and the own notes. Case, accents and hyphens do not
// matter; several words must all match. A technique opens its sheet, a note its notes page.
Kirigami.ScrollablePage {
    id: page

    // ScrollablePage keeps the platform's colours (Theme.inherit: false): give it Kante's, and
    // Kante's page ground (the scope's background role is the dialog colour).
    property Item kanteScope: KanteScope { target: page }
    background: Rectangle { color: KanteStyle.backgroundColor }

    // The query searched (follows the field after a short pause while typing).
    property string query: ""
    readonly property var techniques: query.trim() === "" ? [] : Programme.search(query)
    readonly property var notes: query.trim() === "" || Practice.notesRevision < 0 ? [] : Practice.searchNotes(query)

    title: qsTr("Search")

    header: QQC2.ToolBar {
        KanteTextField {
            id: field

            objectName: "searchField"
            anchors.fill: parent
            placeholderText: qsTr("Technique, attack, Japanese or a word from your notes")
            Accessible.name: qsTr("Search")
            Component.onCompleted: forceActiveFocus()
            onTextChanged: pause.restart()
            onAccepted: page.query = text
        }
    }

    Timer {
        id: pause

        interval: 200
        onTriggered: page.query = field.text
    }

    ColumnLayout {
        spacing: Kirigami.Units.largeSpacing

        QQC2.Label {
            Layout.fillWidth: true
            visible: page.query.trim() !== "" && page.techniques.length === 0 && page.notes.length === 0
            text: qsTr("Nothing found for “%1”.").arg(page.query.trim())
            color: KanteStyle.mutedTextColor
            wrapMode: Text.Wrap
        }

        KanteSectionLabel {
            visible: page.techniques.length > 0
            text: qsTr("Techniques")
        }
        Repeater {
            model: page.techniques

            KanteListRow {
                required property var modelData

                objectName: "techniqueResult"
                Layout.fillWidth: true
                leadingText: modelData.nr ? String(modelData.nr) : ""
                text: modelData.title + (modelData.jp ? "  " + modelData.jp : "")
                subtitle: [modelData.disciplineName + " " + modelData.gradeName, modelData.groupName, modelData.attack].filter(x => x).join(" · ")
                onClicked: {
                    sheet.entry = modelData
                    sheet.playable = !!modelData.video && Videos.available(modelData.video.video)
                    sheet.open()
                }
            }
        }

        KanteSectionLabel {
            visible: page.notes.length > 0
            text: qsTr("Notes")
        }
        Repeater {
            model: page.notes

            KanteListRow {
                required property var modelData
                readonly property var entry: Programme.entry(modelData.entry)

                objectName: "noteResult"
                Layout.fillWidth: true
                text: modelData.text
                subtitle: [entry.title, modelData.created.toLocaleDateString(Qt.locale(), Locale.ShortFormat)].filter(x => x).join(" · ")
                onClicked: page.Kirigami.PageStack.push(Qt.resolvedUrl("NotesPage.qml"), { entry: entry })
            }
        }
    }

    TechniqueSheet {
        id: sheet

        objectName: "techniqueSheet"
        onPlayRequested: entry => {
            sheet.close()
            const window = page.QQC2.ApplicationWindow.window as Kirigami.ApplicationWindow
            window.pageStack.push(Qt.resolvedUrl("PlayerPage.qml"), { playlist: [entry], index: 0 })
        }
        onCommentRequested: entry => page.Kirigami.PageStack.push(Qt.resolvedUrl("CommentPage.qml"), { entry: entry })
        onNotesRequested: entry => page.Kirigami.PageStack.push(Qt.resolvedUrl("NotesPage.qml"), { entry: entry })
    }
}
