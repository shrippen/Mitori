pragma ComponentBehavior: Bound

import QtCore
import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import Kante
import Mitori

// All notes (clickdummy "Alle Notizen"): the notes on every technique by day, newest first, with
// a filter per discipline. A tap opens the technique's notes, press and hold changes or deletes
// a note. Course notebooks join with the courses (ROADMAP.md, step 4).
Kirigami.ScrollablePage {
    id: page

    // ScrollablePage keeps the platform's colours (Theme.inherit: false): give it Kante's, and
    // Kante's page ground (the scope's background role is the dialog colour).
    property Item kanteScope: KanteScope { target: page }
    background: Rectangle { color: KanteStyle.backgroundColor }

    // "" = all disciplines, else a discipline's id; kept for the next time.
    property string filter: ""
    Settings {
        category: "allNotes"
        property alias filter: page.filter
    }
    // Every note with its technique ({}, with only the id, when it is no longer in the programme)
    // and its local day ("YYYY-MM-DD", the list's sections). Newest first.
    readonly property var all: {
        if (Practice.notesRevision < 0) {
            return []
        }
        const p = n => (n < 10 ? "0" : "") + n
        return Practice.allNotes().map(n => {
            const e = Programme.entry(n.entry)
            const c = n.created
            return Object.assign({ technique: e.id ? e : { id: n.entry },
                                   day: c.getFullYear() + "-" + p(c.getMonth() + 1) + "-" + p(c.getDate()) }, n)
        })
    }
    readonly property var shown: filter === "" ? all : all.filter(n => n.technique.discipline === filter)

    title: qsTr("All notes")

    // A section's day ("YYYY-MM-DD"); Practice.today only makes the names follow a new day.
    function dayName(iso) {
        const d = iso.split("-").map(Number)
        const day = new Date(d[0], d[1] - 1, d[2], 12)
        if (Practice.today && day.toDateString() === new Date().toDateString()) {
            return qsTr("Today")
        }
        return day.toLocaleDateString(Qt.locale(), Locale.ShortFormat)
    }
    // "Aikido 5. Kyu" by discipline and grade id.
    readonly property var gradeNames: {
        const names = {}
        for (const d of Programme.disciplines) {
            for (const g of Programme.grades(d.id)) {
                names[d.id + "/" + g.id] = d.kurzname + " " + g.name
            }
        }
        return names
    }
    function techniqueName(e) {
        if (!e.title) {
            return qsTr("Technique no longer in the programme")
        }
        return [page.gradeNames[e.discipline + "/" + e.grade], (e.nr ? e.nr + " · " : "") + e.title, e.attack].filter(x => x).join(" · ")
    }

    // A virtual list (years of notes): the day as its section.
    ListView {
        id: list

        objectName: "notesList"
        model: page.shown
        spacing: Kirigami.Units.smallSpacing
        section.property: "day"
        section.delegate: KanteSectionLabel {
            required property string section
            width: ListView.view.width
            topPadding: Kirigami.Units.largeSpacing
            text: page.dayName(section)
        }

        header: ColumnLayout {
            width: ListView.view.width
            spacing: Kirigami.Units.largeSpacing

            Flow {
                Layout.fillWidth: true
                spacing: Kirigami.Units.smallSpacing

                Repeater {
                    model: [{ id: "", name: qsTr("All") }].concat(Programme.disciplines.map(d => ({ id: d.id, name: d.kurzname })))

                    KanteChip {
                        required property var modelData

                        objectName: "filter_" + (modelData.id || "all")
                        text: modelData.name
                        checkable: true
                        checked: page.filter === modelData.id
                        onClicked: page.filter = modelData.id
                    }
                }
            }

            QQC2.Label {
                objectName: "count"
                Layout.fillWidth: true
                text: page.shown.length > 0
                      ? qsTr("%n note(s). Tap one to open its technique, press and hold to change or delete it.", "", page.shown.length)
                      : page.filter !== "" && page.all.length > 0 ? qsTr("No notes in this discipline.")
                      : qsTr("No notes yet. Notes are written on a technique or in the player.")
                color: KanteStyle.mutedTextColor
                wrapMode: Text.Wrap
            }
        }

        delegate: ColumnLayout {
            id: noteItem

            required property var modelData

            objectName: "note"
            width: ListView.view.width
            spacing: 0
            activeFocusOnTab: true
            Accessible.role: Accessible.Button
            Accessible.name: page.techniqueName(modelData.technique) + ": " + modelData.text
            Accessible.description: qsTr("Opens the technique's notes; press and hold or the menu key to change or delete")
            Accessible.onPressAction: page.open(modelData)
            Keys.onReturnPressed: page.open(modelData)
            Keys.onEnterPressed: page.open(modelData)
            Keys.onMenuPressed: editor.edit(modelData)

            QQC2.Label {
                Layout.fillWidth: true
                text: page.techniqueName(noteItem.modelData.technique)
                color: KanteStyle.mutedTextColor
                font: KanteStyle.smallFont
                elide: Text.ElideRight
                leftPadding: Kirigami.Units.smallSpacing
            }
            QQC2.Label {
                Layout.fillWidth: true
                text: noteItem.modelData.text
                wrapMode: Text.Wrap
                padding: Kirigami.Units.smallSpacing
                background: Rectangle {
                    color: KanteStyle.cardColor
                    border.width: noteItem.activeFocus ? KanteStyle.borderWidth : 0
                    border.color: KanteStyle.focusColor
                }
            }

            TapHandler {
                acceptedButtons: Qt.LeftButton
                onTapped: page.open(noteItem.modelData)
                onLongPressed: editor.edit(noteItem.modelData)
            }
            TapHandler {
                acceptedButtons: Qt.RightButton
                onTapped: editor.edit(noteItem.modelData)
            }
        }
    }

    function open(note) {
        // The notes page's "All notes" comes back here instead of opening another list.
        page.Kirigami.PageStack.push(Qt.resolvedUrl("NotesPage.qml"), {
            entry: note.technique.title ? note.technique : { id: note.entry, title: qsTr("Technique no longer in the programme") },
            fromAllNotes: true
        })
    }

    NoteEditor {
        id: editor
    }
}
