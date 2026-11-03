pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import Kante
import Mitori
import "grades.js" as Grades

// Today's practice (clickdummy "Heute üben"): one discipline per session, up to the grade
// chosen in the programme. Due is what is unsure or long unpractised (days per level, set
// below), and up to two new techniques of the current grade. "Start training" plays them.
Kirigami.ScrollablePage {
    id: page

    // ScrollablePage keeps the platform's colours (Theme.inherit: false): give it Kante's, and
    // Kante's page ground (the scope's background role is the dialog colour).
    property Item kanteScope: KanteScope { target: page }
    background: Rectangle { color: KanteStyle.backgroundColor }

    property string discipline: Programme.discipline
    readonly property var disciplineInfo: Programme.disciplines.find(d => d.id === discipline) || ({})
    readonly property string gradeName: (Programme.grades(discipline).find(g => g.id === Programme.grade(discipline)) || {}).name || ""
    // Re-read after every rating and when the intervals change.
    readonly property var due: Practice.revision >= 0 && Practice.intervals.length === 4 && Practice.today.getTime() > 0
                               ? Practice.due(Programme.practiceEntries(discipline)) : []
    readonly property int unsure: due.filter(r => r.level === 1 || r.level === 2).length
    readonly property int longAgo: due.filter(r => r.level >= 3).length
    readonly property int fresh: due.filter(r => r.level === 0).length
    // The due ones with a video on the device; the hint counts the others (a card in the training).
    readonly property var playable: due.filter(r => !!r.video && Videos.available(r.video.video))
    // An exam planned in this discipline (Courses): its hint leads to the mock exam.
    readonly property var exam: Courses.revision >= 0 ? Courses.nextExam(discipline) : ({})

    title: qsTr("Practice today")

    function reason(row) {
        if (row.level === 0) {
            return qsTr("new in %1").arg(row.gradeName)
        }
        if (row.level <= 2) {
            return qsTr("%1 · %n day(s) ago", "", row.days).arg(Practice.levelNames[row.level])
        }
        return qsTr("not practised for %n day(s)", "", row.days)
    }

    // Every due technique; one without a video on the device shows as a card in the player.
    function startTraining() {
        const playlist = page.due
        if (playlist.length === 0) {
            return
        }
        const window = page.QQC2.ApplicationWindow.window as Kirigami.ApplicationWindow
        window.pageStack.push(Qt.resolvedUrl("PlayerPage.qml"), { playlist: playlist, index: 0, training: true })
    }

    ColumnLayout {
        spacing: Kirigami.Units.largeSpacing

        KanteSectionLabel {
            text: qsTr("Today I practise")
        }
        KanteSegmented {
            Layout.fillWidth: true
            model: Programme.disciplines.map(d => d.kurzname)
            currentIndex: Programme.disciplines.findIndex(d => d.id === page.discipline)
            onActivated: index => page.discipline = Programme.disciplines[index].id
            Accessible.name: qsTr("Discipline")
        }
        QQC2.Label {
            Layout.fillWidth: true
            text: qsTr("up to %1 (as chosen in the programme)").arg(page.gradeName)
            color: KanteStyle.mutedTextColor
            wrapMode: Text.Wrap
        }
        KanteHintCard {
            objectName: "plannedHint"
            Layout.fillWidth: true
            visible: !!page.exam.course
            compact: true
            tier: KanteHintCard.Tier.Primary
            titleText: page.exam.course ? Grades.examText(page.exam) : ""
            meta: page.exam.place || page.exam.title || ""

            KanteButton {
                objectName: "toMockExam"
                // Only for a grade the exam mode has material for (not Shinki Toho Hon-Menkyo).
                visible: !!page.exam.course && Exam.grades(page.exam.discipline).some(g => g.id === page.exam.grade)
                text: qsTr("Mock exam")
                onClicked: {
                    Programme.discipline = page.exam.discipline
                    Programme.setGrade(page.exam.discipline, page.exam.grade)
                    page.Kirigami.PageStack.push(Qt.resolvedUrl("ExamPage.qml"))
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: Kirigami.Units.largeSpacing

            KanteKpi {
                objectName: "dueCount"
                value: String(page.due.length)
                label: qsTr("due")
            }
            QQC2.Label {
                Layout.fillWidth: true
                text: page.due.length === 0 ? qsTr("Everything is practised for today.")
                      : qsTr("%1 unsure · %2 long ago · %3 new").arg(page.unsure).arg(page.longAgo).arg(page.fresh)
                color: KanteStyle.mutedTextColor
                wrapMode: Text.Wrap
            }
        }
        KanteButton {
            visible: page.due.length > 0
            emphasis: KanteButton.Emphasis.Primary
            icon.name: "media-playback-start"
            text: qsTr("Start training")
            onClicked: page.startTraining()
        }
        QQC2.Label {
            Layout.fillWidth: true
            visible: page.due.length > 0 && page.playable.length < page.due.length
            text: qsTr("%n of them without a video on this device: the training shows their name, practise from memory.", "", page.due.length - page.playable.length)
            color: KanteStyle.mutedTextColor
            wrapMode: Text.Wrap
        }

        KanteSectionLabel {
            visible: page.due.length > 0
            text: qsTr("Due")
        }
        Repeater {
            model: page.due

            KanteListRow {
                id: dueRow

                required property var modelData

                Layout.fillWidth: true
                leadingText: modelData.nr ? String(modelData.nr) : ""
                text: modelData.title
                subtitle: [modelData.attack, page.reason(modelData)].filter(x => x).join(" · ")
                onClicked: {
                    sheet.entry = modelData
                    sheet.playable = !!modelData.video && Videos.available(modelData.video.video)
                    sheet.open()
                }
                trailing: KanteLevelDots {
                    value: dueRow.modelData.level
                    names: Practice.levelNames
                    color: KanteStyle.levelColor(dueRow.modelData.level)
                }
            }
        }

        KanteSectionLabel {
            text: qsTr("Intervals · days until due again")
        }
        GridLayout {
            Layout.fillWidth: true
            columns: 2
            columnSpacing: Kirigami.Units.largeSpacing

            Repeater {
                model: 4

                ColumnLayout {
                    id: interval

                    required property int index

                    QQC2.Label {
                        text: (interval.index + 1) + " " + Practice.levelNames[interval.index + 1]
                        color: KanteStyle.mutedTextColor
                    }
                    KanteSpinBox {
                        from: 1
                        to: 365
                        value: Practice.intervals[interval.index]
                        editable: true
                        Accessible.name: qsTr("Days for %1").arg(Practice.levelNames[interval.index + 1])
                        onValueModified: {
                            const days = Practice.intervals.slice()
                            days[interval.index] = value
                            Practice.intervals = days
                        }
                    }
                }
            }
        }
        QQC2.Label {
            Layout.fillWidth: true
            text: qsTr("New techniques of the current grade come on top, at most two a day.")
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
        onVideosRequested: page.Kirigami.PageStack.push(Qt.resolvedUrl("VideosPage.qml"))
    }
}
