import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import Kante
import Mitori
import "coursedays.js" as CourseDays
import "grades.js" as Grades

// Start menu (clickdummy "Startmenü", variant 1): tiles; the programme tile shows the discipline
// and grade chosen last. More tiles come with their features (ROADMAP.md).
Kirigami.ScrollablePage {
    id: page

    // ScrollablePage keeps the platform's colours (Theme.inherit: false): give it Kante's, and
    // Kante's page ground (the scope's background role is the dialog colour).
    property Item kanteScope: KanteScope { target: page }
    background: Rectangle { color: KanteStyle.backgroundColor }

    title: "Mitori 見取り"

    // The course tile names the next date, and updates of the video bundles show here: calendar and
    // manifest load when the last load is a few hours old.
    Component.onCompleted: {
        Courses.reload(Courses.Reload.Older)
        Bundles.reload(Bundles.Reload.Older)
    }

    // A tablet in landscape: a page at least 26 grid units high (a 600 dp tablet less its bars;
    // not a phone held sideways, about 20).
    readonly property bool wide: width > height && height >= Kirigami.Units.gridUnit * 26

    actions: [
        Kirigami.Action {
            icon.name: "search"
            text: qsTr("Search")
            displayHint: Kirigami.DisplayHint.IconOnly
            onTriggered: page.Kirigami.PageStack.push(Qt.resolvedUrl("SearchPage.qml"))
        },
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

        KanteHintCard {
            Layout.fillWidth: true
            visible: Practice.errorString !== ""
            tier: KanteHintCard.Tier.Red
            titleText: qsTr("Ratings are not saved")
            text: Practice.errorString
        }

        // A new version of a loaded video bundle (its own hint, apart from the app's updates).
        KanteHintCard {
            objectName: "bundleUpdate"
            Layout.fillWidth: true
            visible: Bundles.updates > 0
            tier: KanteHintCard.Tier.Blue
            titleText: qsTr("New video version")
            text: qsTr("%n video bundle(s) can be updated.", "", Bundles.updates)

            KanteButton {
                text: qsTr("Videos")
                onClicked: page.Kirigami.PageStack.push(Qt.resolvedUrl("VideosPage.qml"))
            }
        }

        // A column on the phone, a grid of four on a tablet in landscape (app-entscheidungen.md).
        GridLayout {
            objectName: "tiles"
            Layout.fillWidth: true
            columns: page.wide ? 4 : 1
            rowSpacing: Kirigami.Units.largeSpacing
            columnSpacing: Kirigami.Units.largeSpacing

            KanteOptionTile {
                id: programmeTile

                readonly property var current: page.disciplineInfo(Programme.discipline)

                Layout.fillWidth: true
                Layout.preferredWidth: 1          // equal columns
                visible: Programme.errorString === ""
                checkable: false
                autoExclusive: false
                glyph: "技"
                text: qsTr("Programme")
                hint: current.title + " · " + current.grade
                onClicked: page.Kirigami.PageStack.push(Qt.resolvedUrl("ProgrammePage.qml"))
            }

            KanteOptionTile {
                objectName: "todayTile"

                readonly property int due: Practice.revision >= 0 && Practice.intervals.length === 4 && Practice.today.getTime() > 0
                                           ? Practice.due(Programme.practiceEntries(Programme.discipline)).length : 0

                Layout.fillWidth: true
                Layout.preferredWidth: 1          // equal columns
                visible: Programme.errorString === ""
                checkable: false
                autoExclusive: false
                glyph: "稽"
                text: qsTr("Practice today")
                hint: due > 0 ? qsTr("%n due", "", due) : qsTr("all practised")
                onClicked: page.Kirigami.PageStack.push(Qt.resolvedUrl("TodayPage.qml"))
            }

            KanteOptionTile {
                objectName: "examTile"

                // Programme.discipline notifies on a new grade too.
                readonly property string grade: Exam.grade(Programme.discipline)
                readonly property string gradeName: (Exam.grades(Programme.discipline).find(g => g.id === grade) || {}).name || ""

                Layout.fillWidth: true
                Layout.preferredWidth: 1          // equal columns
                visible: Programme.errorString === ""
                checkable: false
                autoExclusive: false
                glyph: "試"
                text: qsTr("Exam mode")
                hint: qsTr("Mock exam %1").arg(gradeName)
                onClicked: page.Kirigami.PageStack.push(Qt.resolvedUrl("ExamPage.qml"))
            }

            KanteOptionTile {
                objectName: "statsTile"

                readonly property int days: Practice.revision >= 0 && Practice.today.getTime() > 0
                                            ? Object.keys(Practice.practisedPerDay(30)).length : 0

                Layout.fillWidth: true
                Layout.preferredWidth: 1          // equal columns
                visible: Programme.errorString === ""
                checkable: false
                autoExclusive: false
                glyph: "記"
                text: qsTr("Statistics")
                hint: qsTr("%n practice day(s) in 30 days", "", days)
                onClicked: page.Kirigami.PageStack.push(Qt.resolvedUrl("StatsPage.qml"))
            }

            KanteOptionTile {
                objectName: "coursesTile"

                // A planned exam first ("Exam Aikido 4. Kyu in 30 days"), else the next coming
                // date: "Siebeldingen · in 12 days", "today".
                readonly property var exam: Courses.revision >= 0 ? Courses.nextExam() : ({})
                readonly property var next: Courses.upcoming.length > 0 ? Courses.upcoming[0] : null
                readonly property int days: next ? Math.max(0, CourseDays.fromToday(next.first)) : 0

                Layout.fillWidth: true
                Layout.preferredWidth: 1          // equal columns
                checkable: false
                autoExclusive: false
                glyph: "講"
                text: qsTr("Courses")
                hint: exam.course ? Grades.examText(exam)
                    : !next ? qsTr("Calendar and notebooks")
                            : (next.place || next.title) + " · " + (days > 0 ? qsTr("in %n day(s)", "", days) : qsTr("today"))
                onClicked: page.Kirigami.PageStack.push(Qt.resolvedUrl("CoursesPage.qml"))
            }

            KanteOptionTile {
                objectName: "mapTile"
                Layout.fillWidth: true
                Layout.preferredWidth: 1          // equal columns
                visible: Programme.errorString === ""
                checkable: false
                autoExclusive: false
                glyph: "図"
                text: qsTr("Technique map")
                hint: qsTr("Attack × technique")
                onClicked: page.Kirigami.PageStack.push(Qt.resolvedUrl("TechniqueMapPage.qml"))
            }

            KanteOptionTile {
                objectName: "glossaryTile"

                Layout.fillWidth: true
                Layout.preferredWidth: 1          // equal columns
                visible: Programme.errorString === ""
                checkable: false
                autoExclusive: false
                glyph: "辞"
                text: qsTr("Glossary")
                hint: qsTr("Kanji · pronunciation")
                onClicked: page.Kirigami.PageStack.push(Qt.resolvedUrl("SearchPage.qml"))
            }
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
