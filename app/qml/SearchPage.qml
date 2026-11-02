pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import Kante
import Mitori

// Search (clickdummy "Glossar & Suche", the search part): techniques of every discipline and
// grade by name, attack (with its addition), group or Japanese, and the own notes. Case,
// accents, hyphens and okurigana do not matter; several words must all match. A technique opens its sheet, a note its notes page.
Kirigami.ScrollablePage {
    id: page

    // ScrollablePage keeps the platform's colours (Theme.inherit: false): give it Kante's, and
    // Kante's page ground (the scope's background role is the dialog colour).
    property Item kanteScope: KanteScope { target: page }
    background: Rectangle { color: KanteStyle.backgroundColor }

    // The query searched (follows the field after a short pause while typing).
    property string query: ""
    // At most `limit` techniques are listed; one more found says there are more.
    readonly property int limit: 50
    readonly property var techniques: query.trim() === "" ? [] : Programme.search(query, limit + 1)
    readonly property bool more: techniques.length > limit
    readonly property var shown: more ? techniques.slice(0, limit) : techniques
    readonly property var notes: query.trim() === "" || Practice.notesRevision < 0 ? [] : Practice.searchNotes(query, limit + 1)
    readonly property bool notesMore: notes.length > limit
    readonly property var notesShown: notesMore ? notes.slice(0, limit) : notes

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
            model: page.shown

            KanteListRow {
                required property var modelData

                objectName: "techniqueResult"
                Layout.fillWidth: true
                leadingText: modelData.nr ? String(modelData.nr) : ""
                // As in the programme: the technique's and the attack's addition tell rows apart.
                text: modelData.title + (modelData.extra && !modelData.nr ? " (" + modelData.extra + ")" : "")
                      + (modelData.jp ? "  " + modelData.jp : "")
                subtitle: [modelData.disciplineName + " " + modelData.gradeName, modelData.groupName,
                           modelData.attack + (modelData.attackExtra ? " (" + modelData.attackExtra + ")" : ""),
                           modelData.nr ? modelData.extra : ""].filter(x => x).join(" · ")
                onClicked: {
                    sheet.entry = modelData
                    sheet.playable = !!modelData.video && Videos.available(modelData.video.video)
                    sheet.open()
                }
            }
        }

        QQC2.Label {
            objectName: "moreHint"
            Layout.fillWidth: true
            visible: page.more
            text: qsTr("More than %1 techniques found. Add a word to narrow the search.").arg(page.limit)
            color: KanteStyle.mutedTextColor
            wrapMode: Text.Wrap
        }

        KanteSectionLabel {
            visible: page.notes.length > 0
            text: qsTr("Notes")
        }
        Repeater {
            model: page.notesShown

            KanteListRow {
                required property var modelData
                // A technique no longer in the programme (a newer catalogue) keeps its notes.
                readonly property var found: Programme.entry(modelData.entry)
                readonly property var entry: found.id ? found : { id: modelData.entry, title: qsTr("Technique no longer in the programme") }

                objectName: "noteResult"
                Layout.fillWidth: true
                text: modelData.text.replace(/\s+/g, " ")       // one line, as the row shows it
                subtitle: [entry.title, modelData.created.toLocaleDateString(Qt.locale(), Locale.ShortFormat)].filter(x => x).join(" · ")
                onClicked: page.Kirigami.PageStack.push(Qt.resolvedUrl("NotesPage.qml"), { entry: entry })
            }
        }

        QQC2.Label {
            objectName: "notesMoreHint"
            Layout.fillWidth: true
            visible: page.notesMore
            text: qsTr("More than %1 notes found. Add a word to narrow the search.").arg(page.limit)
            color: KanteStyle.mutedTextColor
            wrapMode: Text.Wrap
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
