import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import Mitori
import "coursedays.js" as CourseDays

// A course in a list: the first day as a date block, title, place · days · teacher, and how
// soon it is and how many notes its notebook has.
//   [ 6 ]  Daitoryu Aiki Jujutsu                       in 12 days
//    Jun   Siebeldingen · 06.06.26 – 07.06.26 · …       3 notes
LookListRow {
    id: row

    // {title, first, last, place, teacher, entries} as Courses gives an event or a notebook.
    required property var course
    readonly property int daysAway: CourseDays.fromToday(course.first)
    readonly property date firstDay: CourseDays.day(course.first)

    leadingText: firstDay.getDate() + " " + firstDay.toLocaleDateString(Qt.locale(), "MMM")
    text: course.title
    subtitle: [course.place, CourseDays.range(course.first, course.last), course.teacher].filter(x => x).join(" · ")
    trailing: ColumnLayout {
        spacing: 0

        QQC2.Label {
            Layout.alignment: Qt.AlignRight
            visible: row.daysAway > 0
            text: qsTr("in %n day(s)", "", row.daysAway)
            color: Look.mutedTextColor
            font: Look.smallFont
        }
        QQC2.Label {
            Layout.alignment: Qt.AlignRight
            visible: row.course.entries > 0
            text: qsTr("%n note(s)", "", row.course.entries)
            color: Look.mutedTextColor
            font: Look.smallFont
        }
    }
}
