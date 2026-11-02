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
Kirigami.Page {
    id: page

    // Kante's colours and page ground (the scope's background role is the dialog colour).
    property Item kanteScope: KanteScope { target: page }
    background: Rectangle { color: KanteStyle.backgroundColor }

    // "" = all disciplines, else a discipline's id; kept for the next time.
    property string filter: ""
    Settings {
        id: settings

        property alias allNotesFilter: page.filter
    }
    // A remembered discipline that no longer exists: all (also when Settings loads it). Another
    // filter starts at the top of the list.
    onFilterChanged: {
        if (filter !== "" && !Programme.disciplines.some(d => d.id === filter)) {
            Qt.callLater(() => page.filter = "")     // not inside the change (Settings records it)
        }
        settings.setValue("allNotesFilter", filter)  // now, not a moment later (the page may go)
        if (filter !== listFilter) {
            listFilter = filter
            Qt.callLater(() => list.positionViewAtBeginning())
        }
    }
    // The filter the list was last at the top for (Settings reports the same filter again).
    property string listFilter: ""
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
    // The list's model: a new `shown` changes only the rows that changed, so the list keeps its
    // place and its current note after an edit, a deletion or a new note.
    KeyedListModel {
        id: notesModel

        rows: page.shown
    }

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

    padding: 0

    // The filter, then the list: in this order also for Tab.
    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Flow {
            Layout.fillWidth: true
            padding: Kirigami.Units.smallSpacing
            leftPadding: Kirigami.Units.largeSpacing
            rightPadding: Kirigami.Units.largeSpacing
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

        QQC2.ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.leftMargin: Kirigami.Units.largeSpacing
            Layout.rightMargin: Kirigami.Units.largeSpacing

            // A virtual list (years of notes): the day as its section.
            ListView {
                id: list

                objectName: "notesList"
                model: notesModel
                spacing: Kirigami.Units.smallSpacing
                // The keyboard moves through every note (Up, Down), Enter opens, the menu key edits; the
                // list follows the current note.
                activeFocusOnTab: true
                keyNavigationEnabled: true
                currentIndex: -1
                onCurrentIndexChanged: {
                    if (activeFocus && currentIndex >= 0) {
                        positionViewAtIndex(currentIndex, ListView.Contain)
                    }
                }
                // Coming in with Tab: the first note in view, not one far away.
                onActiveFocusChanged: {
                    if (activeFocus && (currentIndex < 0 || currentIndex < firstShown() || currentIndex > lastShown())) {
                        currentIndex = firstShown()
                    }
                }
                // The first and last note in view (a day's heading or the gap between notes is no note).
                function firstShown() {
                    for (let y = contentY; y < contentY + height; y += Kirigami.Units.smallSpacing) {
                        const i = indexAt(width / 2, y)
                        if (i >= 0) {
                            return i
                        }
                    }
                    return count > 0 ? 0 : -1
                }
                function lastShown() {
                    for (let y = contentY + height - 1; y > contentY; y -= Kirigami.Units.smallSpacing) {
                        const i = indexAt(width / 2, y)
                        if (i >= 0) {
                            return i
                        }
                    }
                    return firstShown()
                }
                Keys.onReturnPressed: page.openCurrent(false)
                Keys.onEnterPressed: page.openCurrent(false)
                Keys.onMenuPressed: page.openCurrent(true)
                Keys.onPressed: event => {
                    const page_ = Math.max(1, lastShown() - firstShown())
                    if (event.key === Qt.Key_PageDown) {
                        currentIndex = Math.min(count - 1, currentIndex + page_)
                    } else if (event.key === Qt.Key_PageUp) {
                        currentIndex = Math.max(0, currentIndex - page_)
                    } else if (event.key === Qt.Key_Home) {
                        currentIndex = 0
                    } else if (event.key === Qt.Key_End) {
                        currentIndex = count - 1
                    } else {
                        return
                    }
                    event.accepted = true
                }
                section.property: "day"
                section.delegate: KanteSectionLabel {
                    required property string section
                    Accessible.role: Accessible.Heading
                    width: ListView.view.width
                    topPadding: Kirigami.Units.largeSpacing
                    text: page.dayName(section)
                }

                header: QQC2.Label {
                    objectName: "count"
                    width: ListView.view.width
                    bottomPadding: Kirigami.Units.largeSpacing
                    text: page.shown.length > 0
                          ? qsTr("%n note(s). Tap one to open its technique, press and hold to change or delete it.", "", page.shown.length)
                          : page.filter !== "" && page.all.length > 0 ? qsTr("No notes in this discipline.")
                          : qsTr("No notes yet. Notes are written on a technique or in the player.")
                    color: KanteStyle.mutedTextColor
                    wrapMode: Text.Wrap
                }

                delegate: ColumnLayout {
                    id: noteItem

                    required property var row
                    required property int index

                    objectName: "note"

                    width: ListView.view.width
                    spacing: 0
                    readonly property bool current: ListView.isCurrentItem && ListView.view.activeFocus
                    Accessible.role: Accessible.Button
                    Accessible.name: page.techniqueName(row.technique) + ": " + row.text
                    Accessible.description: qsTr("Opens the technique's notes; press and hold or the menu key to change or delete")
                    Accessible.onPressAction: page.open(row)

                    QQC2.Label {
                        Layout.fillWidth: true
                        text: page.techniqueName(noteItem.row.technique)
                        color: KanteStyle.mutedTextColor
                        font: KanteStyle.smallFont
                        elide: Text.ElideRight
                        leftPadding: Kirigami.Units.smallSpacing
                    }
                    QQC2.Label {
                        Layout.fillWidth: true
                        text: noteItem.row.text
                        wrapMode: Text.Wrap
                        padding: Kirigami.Units.smallSpacing
                        background: Rectangle {
                            color: KanteStyle.cardColor
                            border.width: noteItem.current ? KanteStyle.borderWidth : 0
                            border.color: KanteStyle.focusColor
                        }
                    }

                    TapHandler {
                        acceptedButtons: Qt.LeftButton
                        onTapped: page.open(noteItem.row)
                        onLongPressed: editor.edit(noteItem.row)
                    }
                    TapHandler {
                        acceptedButtons: Qt.RightButton
                        onTapped: editor.edit(noteItem.row)
                    }
                }
            }
        }
    }

    // Enter or the menu key on the note the keyboard is on.
    function openCurrent(edit) {
        const note = page.shown[list.currentIndex]
        if (!note) {
            return
        }
        if (edit) {
            editor.returnFocus = list
            editor.edit(note)
        } else {
            page.open(note)
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

        // Edited with the keyboard: the list gets the focus back, on the same note.
        property Item returnFocus: null
        onClosed: {
            if (returnFocus) {
                returnFocus.forceActiveFocus()
                returnFocus = null
            }
        }
    }
}
