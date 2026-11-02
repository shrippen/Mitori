pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import Kante
import Mitori

// Technique map (clickdummy "Landkarte"): every grade up to the chosen one, one group at a time
// (app-entscheidungen.md: groups apart), techniques against attacks, each place coloured by how
// well it sits; Shinki Toho as the numbered Ô-dachi forms. A place opens the technique sheet.
Kirigami.ScrollablePage {
    id: page

    // ScrollablePage keeps the platform's colours (Theme.inherit: false): give it Kante's, and
    // Kante's page ground (the scope's background role is the dialog colour).
    property Item kanteScope: KanteScope { target: page }
    background: Rectangle { color: KanteStyle.backgroundColor }
    // Tab and the arrow keys keep the focused place in view.
    property KanteFocusFollow focusFollow: KanteFocusFollow { flickable: page.flickable }

    property string discipline: Programme.discipline
    readonly property var grades: Programme.grades(discipline)
    // Up to which grade; the programme's choice at first.
    property string grade: Programme.grade(discipline)
    readonly property string gradeName: (grades.find(g => g.id === grade) || {}).name || ""
    readonly property var grids: Programme.techniqueMap(discipline, grade)
    // The group shown, by title; the first one when the chosen one has nothing up to this grade.
    property string group: ""
    readonly property var shown: grids.find(g => g.title === group) || grids[0] || null
    readonly property bool matrix: !!shown && shown.kind === "matrix"
    // A tablet in landscape: a page at least 26 grid units high (a phone held sideways has about 20).
    readonly property bool wide: width > height && height >= Kirigami.Units.gridUnit * 26

    title: qsTr("Technique map")

    ColumnLayout {
        spacing: Kirigami.Units.largeSpacing

        KanteSegmented {
            Layout.fillWidth: true
            model: Programme.disciplines.map(d => d.kurzname)
            currentIndex: Programme.disciplines.findIndex(d => d.id === page.discipline)
            onActivated: index => {
                page.discipline = Programme.disciplines[index].id
                page.grade = Programme.grade(page.discipline)
            }
        }

        Flow {
            Layout.fillWidth: true
            spacing: Kirigami.Units.smallSpacing

            Repeater {
                model: page.grades

                KanteChip {
                    required property var modelData

                    objectName: "gradeChip"
                    text: qsTr("up to %1").arg(modelData.name)
                    checkable: true
                    checked: modelData.id === page.grade
                    onClicked: page.grade = modelData.id
                }
            }
        }

        // Groups apart: one grid at a time.
        Flow {
            Layout.fillWidth: true
            visible: page.matrix
            spacing: Kirigami.Units.smallSpacing

            Repeater {
                model: page.matrix ? page.grids : []

                KanteChip {
                    required property var modelData

                    objectName: "groupChip"
                    text: modelData.title
                    checkable: true
                    checked: !!page.shown && modelData.title === page.shown.title
                    onClicked: page.group = modelData.title
                }
            }
        }

        QQC2.Label {
            Layout.fillWidth: true
            visible: !page.shown
            text: qsTr("No techniques with an attack up to %1.").arg(page.gradeName)
            color: KanteStyle.mutedTextColor
            wrapMode: Text.Wrap
        }

        KanteSectionLabel {
            visible: !!page.shown
            text: page.shown ? page.shown.title + (page.shown.jp ? "  " + page.shown.jp : "") : ""
        }

        TechniqueMatrix {
            objectName: "techniqueMatrix"
            Layout.fillWidth: true
            visible: page.matrix
            grid: page.matrix ? page.shown : ({})
            cellSize: page.wide ? KanteStyle.unit(34) : KanteStyle.unit(26)
            onActivated: cell => page.open(cell.entry)
        }

        FormGrid {
            objectName: "formGrid"
            Layout.fillWidth: true
            visible: !!page.shown && !page.matrix
            cells: page.shown && !page.matrix ? page.shown.cells : []
            columns: page.wide ? 11 : 8
            onActivated: cell => page.open(cell.entry)
        }

        KanteLegend {
            Layout.fillWidth: true
            visible: !!page.shown
            model: [0, 1, 2, 3, 4].map(l => ({
                color: KanteStyle.levelColor(l),
                value: l > 0 && page.matrix ? String(l) : undefined,
                text: Practice.levelNames[l]
            }))
        }
        QQC2.Label {
            Layout.fillWidth: true
            visible: !!page.shown
            text: page.matrix
                  ? qsTr("Rows: technique, columns: attack. An empty place does not occur up to %1. Tap a place to open the technique.").arg(page.gradeName)
                  : qsTr("A place per form, numbered. Tap a place to open the form.")
            color: KanteStyle.mutedTextColor
            font: KanteStyle.smallFont
            wrapMode: Text.Wrap
        }
    }

    function open(entry) {
        sheet.entry = entry
        sheet.playable = !!entry.video && Videos.available(entry.video.video)
        sheet.open()
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
