pragma ComponentBehavior: Bound

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

    // "" = all disciplines, else a discipline's id.
    property string filter: ""
    // Every note with its technique ({}, with only the id, when it is no longer in the programme).
    readonly property var all: {
        if (Practice.notesRevision < 0) {
            return []
        }
        return Practice.allNotes().map(n => {
            const e = Programme.entry(n.entry)
            return Object.assign({ technique: e.id ? e : { id: n.entry } }, n)
        })
    }
    readonly property var shown: filter === "" ? all : all.filter(n => n.technique.discipline === filter)
    // [{key, day, notes}], newest day first; within a day newest first.
    readonly property var days: {
        const result = []
        for (const n of shown) {
            const key = n.created.toDateString()
            let day = result.find(d => d.key === key)
            if (!day) {
                day = { key: key, day: n.created, notes: [] }
                result.push(day)
            }
            day.notes.push(n)
        }
        return result
    }

    title: qsTr("All notes")

    // Practice.today only makes the names follow a new day.
    function dayName(day) {
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

    ColumnLayout {
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
            text: page.shown.length === 0 ? qsTr("No notes yet. Notes are written on a technique or in the player.")
                                           : qsTr("%n note(s). Tap one to open its technique, press and hold to change or delete it.", "", page.shown.length)
            color: KanteStyle.mutedTextColor
            wrapMode: Text.Wrap
        }

        Repeater {
            model: page.days

            ColumnLayout {
                id: dayBlock

                required property var modelData

                Layout.fillWidth: true
                spacing: Kirigami.Units.smallSpacing

                KanteSectionLabel {
                    text: page.dayName(dayBlock.modelData.day)
                }
                Repeater {
                    model: dayBlock.modelData.notes

                    ColumnLayout {
                        id: noteItem

                        required property var modelData

                        objectName: "note"
                        Layout.fillWidth: true
                        spacing: 0
                        activeFocusOnTab: true
                        Accessible.role: Accessible.Button
                        Accessible.name: page.techniqueName(modelData.technique) + ": " + modelData.text
                        Accessible.description: qsTr("Opens the technique's notes; press and hold to change or delete")
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
            }
        }
    }

    function open(note) {
        page.Kirigami.PageStack.push(Qt.resolvedUrl("NotesPage.qml"), { entry: note.technique.title ? note.technique
                                     : { id: note.entry, title: qsTr("Technique no longer in the programme") } })
    }

    NoteEditor {
        id: editor
    }
}
