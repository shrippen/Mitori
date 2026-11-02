pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import Kante
import Mitori
import "coursedays.js" as CourseDays
import "grades.js" as Grades

// My grades (clickdummy "Meine Grade"): per discipline the grades passed with day and place, the
// next grade with its waiting time (from the programme data) and a planned exam. They are the
// ground for the waiting times on a course and the hints before an exam.
//
//   AIKIDO · last 4. Kyu
//   4. Kyu        09.03.25 · Mainz                         [×]
//   Next: 3. Kyu · from 09.03.26 · planned: Bonn, 12.06.26
//   [3. Kyu ▾] [2026-11-02] [Place      ] [Add]
Kirigami.ScrollablePage {
    id: page

    // ScrollablePage keeps the platform's colours (Theme.inherit: false): give it Kante's, and
    // Kante's page ground (the scope's background role is the dialog colour).
    property Item kanteScope: KanteScope { target: page }
    background: Rectangle { color: KanteStyle.backgroundColor }

    readonly property var grades: Courses.revision >= 0 ? Courses.grades : []

    title: qsTr("My grades")

    ColumnLayout {
        spacing: Kirigami.Units.largeSpacing

        QQC2.Label {
            Layout.fillWidth: true
            visible: Courses.errorString !== ""
            text: Courses.errorString
            color: KanteStyle.negativeTextColor
            wrapMode: Text.Wrap
        }

        Repeater {
            model: Programme.disciplines

            ColumnLayout {
                id: block

                required property var modelData
                readonly property string discipline: modelData.id
                readonly property var passed: page.grades.filter(g => g.discipline === discipline)
                readonly property var ladder: Courses.revision >= 0 ? Courses.ladder(discipline) : []
                readonly property string next: Courses.revision >= 0 ? Courses.nextGrade(discipline) : ""
                readonly property var exam: Courses.revision >= 0 ? Courses.nextExam(discipline) : ({})

                objectName: "grades_" + discipline
                Layout.fillWidth: true
                spacing: Kirigami.Units.smallSpacing

                KanteSectionLabel {
                    Layout.fillWidth: true
                    rule: true
                    text: block.modelData.kurzname + " · "
                          + (block.passed.length > 0 ? qsTr("last %1").arg(block.passed[0].name) : qsTr("no grade yet"))
                }

                Repeater {
                    model: block.passed

                    KanteListRow {
                        id: passedRow

                        required property var modelData

                        objectName: "passedRow"
                        Layout.fillWidth: true
                        text: modelData.name
                        subtitle: [Grades.shortDate(modelData.day), modelData.place].filter(x => x).join(" · ")
                        trailing: KanteToolButton {
                            objectName: "removeGrade"
                            icon.name: "edit-delete"
                            text: qsTr("Remove %1").arg(passedRow.modelData.name)
                            display: QQC2.AbstractButton.IconOnly
                            onClicked: Courses.removeGrade(passedRow.modelData.uid)
                        }
                    }
                }

                QQC2.Label {
                    objectName: "nextGrade"
                    Layout.fillWidth: true
                    visible: block.next !== ""
                    text: {
                        if (block.next === "") {
                            return ""
                        }
                        const name = (block.ladder.find(g => g.id === block.next) || {}).name || ""
                        const parts = [qsTr("Next: %1").arg(name), Grades.waitText(Courses.waiting(block.discipline, block.next))]
                        if (block.exam.course) {
                            parts.push(qsTr("planned: %1, %2").arg(block.exam.place || block.exam.title).arg(Grades.shortDate(block.exam.first)))
                        }
                        return parts.join(" · ")
                    }
                    color: KanteStyle.mutedTextColor
                    wrapMode: Text.Wrap
                }

                // A passed grade: the next one is chosen, today's day.
                Flow {
                    Layout.fillWidth: true
                    spacing: Kirigami.Units.smallSpacing

                    KanteComboBox {
                        id: gradeBox

                        objectName: "gradeBox"
                        model: block.ladder.map(g => g.name)
                        currentIndex: Math.max(0, block.ladder.findIndex(g => g.id === block.next))
                        Accessible.name: qsTr("Grade")
                    }
                    KanteDateField {
                        id: dayField

                        objectName: "gradeDay"
                        format: qsTr("yyyy-MM-dd", "date format")
                        dayNames: [qsTr("MO"), qsTr("TU"), qsTr("WE"), qsTr("TH"), qsTr("FR"), qsTr("SA"), qsTr("SU")]
                        todayText: qsTr("Today")
                        date: new Date()
                        Accessible.name: qsTr("Day of the exam")
                    }
                    KanteTextField {
                        id: placeField

                        objectName: "gradePlace"
                        placeholderText: qsTr("Place")
                        Accessible.name: qsTr("Place")
                    }
                    KanteButton {
                        objectName: "addGrade"
                        icon.name: "list-add"
                        text: qsTr("Add")
                        enabled: block.ladder.length > 0 && !!dayField.date && !dayField.invalid
                        onClicked: {
                            const grade = block.ladder[gradeBox.currentIndex]
                            if (grade && Courses.addGrade(block.discipline, grade.id, CourseDays.iso(dayField.date), placeField.text)) {
                                placeField.clear()
                            }
                        }
                    }
                }
            }
        }

        QQC2.Label {
            Layout.fillWidth: true
            text: qsTr("The ground for waiting times and the hints before an exam. The same grade again corrects its day and place.")
            color: KanteStyle.mutedTextColor
            wrapMode: Text.Wrap
        }
    }
}
