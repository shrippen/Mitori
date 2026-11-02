pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import Kante
import Mitori

// Statistics (clickdummy "Statistik"): the last 30 days in two figures, the practice days of the
// last half year on a calendar, and how each grade of a discipline sits. A technique counts once
// a day it was rated; the average of the last exam rounds of the discipline. "My grades" come
// with their feature (ROADMAP.md).
Kirigami.ScrollablePage {
    id: page

    // ScrollablePage keeps the platform's colours (Theme.inherit: false): give it Kante's, and
    // Kante's page ground (the scope's background role is the dialog colour).
    property Item kanteScope: KanteScope { target: page }
    background: Rectangle { color: KanteStyle.backgroundColor }

    // {"YYYY-MM-DD": techniques} for half a year; re-read after a rating and on a new day.
    readonly property var perDay: Practice.revision >= 0 && Practice.today.getTime() > 0 ? Practice.practisedPerDay(26 * 7) : ({})
    // Today as a date; reading Practice.today makes it follow midnight.
    readonly property date todayDate: {
        Practice.today
        return new Date()
    }
    readonly property var last30: {
        const first = new Date(page.todayDate)
        first.setDate(first.getDate() - 29)
        const from = heat.isoDay(first)
        const days = Object.keys(page.perDay).filter(d => d >= from)
        return { days: days.length, techniques: days.reduce((sum, d) => sum + page.perDay[d], 0) }
    }
    property string discipline: Programme.discipline
    // The last eight exam rounds of the discipline, oldest first.
    readonly property var rounds: Practice.revision >= 0 ? Practice.rounds(discipline, 8) : []

    function averageText(value) {
        const rounded = Math.round(value * 10) / 10
        return Number.isInteger(rounded) ? String(rounded) : rounded.toLocaleString(Qt.locale(), "f", 1)
    }
    function roundText(round) {
        const grade = Programme.grades(page.discipline).find(g => g.id === round.grade)
        return [grade ? grade.name : round.grade, qsTr("%n technique(s)", "", round.count), qsTr("average")].join(" · ")
    }

    // Techniques on a day as a level of the calendar: 1–2, 3–5, 6–9, 10 and more (finer than the
    // clickdummy's 1–4 … 13+: a session rates a handful of techniques).
    readonly property var bounds: [1, 3, 6, 10]
    function levelOf(count) {
        return page.bounds.filter(b => count >= b).length
    }

    title: qsTr("Statistics")

    ColumnLayout {
        spacing: Kirigami.Units.largeSpacing

        KanteSectionLabel {
            text: qsTr("Last 30 days")
        }
        RowLayout {
            Layout.fillWidth: true
            spacing: Kirigami.Units.gridUnit * 2

            KanteKpi {
                objectName: "practiceDays"
                value: String(page.last30.days)
                label: qsTr("practice days")
            }
            KanteKpi {
                objectName: "techniquesPractised"
                value: String(page.last30.techniques)
                label: qsTr("techniques practised")
            }
        }
        QQC2.Label {
            Layout.fillWidth: true
            text: qsTr("A technique counts once on each day it is rated.")
            color: KanteStyle.mutedTextColor
            font: KanteStyle.smallFont
            wrapMode: Text.Wrap
        }

        KanteSectionLabel {
            text: qsTr("Practice days")
        }
        Item {
            id: heatBox

            Layout.fillWidth: true
            implicitHeight: heat.implicitHeight

            KanteCalendarHeatmap {
                id: heat

                objectName: "practiceHeatmap"
                // As many weeks as fit, up to half a year.
                weeks: Math.max(4, Math.min(26, Math.floor((heatBox.width - labelWidth + spacing) / step)))
                endDate: page.todayDate
                levels: {
                    const l = {}
                    for (const d in page.perDay) {
                        l[d] = page.levelOf(page.perDay[d])
                    }
                    return l
                }
                // Every day shown says what it was, also a day without practice.
                tips: {
                    const t = {}
                    for (let i = 0; i < 26 * 7; i++) {
                        const day = new Date(page.todayDate.getFullYear(), page.todayDate.getMonth(), page.todayDate.getDate() - i, 12)
                        const key = isoDay(day)
                        t[key] = page.perDay[key] ? qsTr("%n technique(s)", "", page.perDay[key]) : qsTr("no practice")
                    }
                    return t
                }
                Accessible.name: qsTr("Practice days")
            }
        }
        KanteLegend {
            Layout.fillWidth: true
            model: [{ color: KanteStyle.levelColor(0), text: qsTr("no practice") },
                    { color: KanteStyle.levelColor(1), text: qsTr("1–2 techniques") },
                    { color: KanteStyle.levelColor(2), text: "3–5" },
                    { color: KanteStyle.levelColor(3), text: "6–9" },
                    { color: KanteStyle.levelColor(4), text: qsTr("10 and more") }]
        }

        KanteSectionLabel {
            text: qsTr("Rating by grade")
        }
        KanteSegmented {
            Layout.fillWidth: true
            model: Programme.disciplines.map(d => d.kurzname)
            currentIndex: Programme.disciplines.findIndex(d => d.id === page.discipline)
            onActivated: index => page.discipline = Programme.disciplines[index].id
        }
        Repeater {
            model: Programme.grades(page.discipline)

            ColumnLayout {
                id: gradeItem

                required property var modelData
                readonly property var rows: Programme.entries(page.discipline, modelData.id)

                Layout.fillWidth: true
                spacing: KanteStyle.unit(4)
                visible: stand.ids.length > 0

                RowLayout {
                    Layout.fillWidth: true

                    QQC2.Label {
                        Layout.fillWidth: true
                        text: gradeItem.modelData.name
                        font.bold: true
                    }
                    // The figures without hovering (touch): exam-ready of all techniques.
                    QQC2.Label {
                        objectName: "gradeFigure"
                        text: qsTr("%1 of %2 exam-ready").arg(stand.counts[4]).arg(stand.ids.length)
                        color: KanteStyle.mutedTextColor
                    }
                }
                GradeStand {
                    id: stand

                    objectName: "statsGradeStand"
                    Layout.fillWidth: true
                    entries: gradeItem.rows
                    name: gradeItem.modelData.name
                    legend: false
                }
            }
        }
        KanteLegend {
            Layout.fillWidth: true
            model: [4, 3, 2, 1, 0].map(l => ({ color: KanteStyle.levelColor(l), text: Practice.levelNames[l] }))
        }

        // Exam mode: the average rating of each of the last rounds of the discipline, on a scale
        // from 1 (seen) to 4 (exam-ready). A round of all 1 keeps a sliver (minBarHeight), else it
        // would show no bar at all.
        KanteSectionLabel {
            text: qsTr("Exam mode · average per round")
        }
        KanteBarChart {
            id: examChart

            objectName: "examChart"
            Layout.fillWidth: true
            Layout.preferredHeight: KanteStyle.unit(160)
            visible: page.rounds.length > 0
            values: page.rounds.map(r => r.average)
            labels: page.rounds.map(r => r.at.toLocaleDateString(Qt.locale(), qsTr("M/d", "date under a bar")))
            minValue: 1
            maxValue: 4
            minBarHeight: KanteStyle.unit(3)
            axis: true
            axisTitle: qsTr("Average")
            formatter: v => page.averageText(v)
            // The read-out names the round: date (its title), grade, techniques and average.
            partNames: hoverIndex >= 0 && hoverIndex < page.rounds.length ? [page.roundText(page.rounds[hoverIndex])] : []
            Accessible.name: qsTr("Exam mode, average per round")

            // Touch has no hover: a tap shows the read-out of its bar.
            TapHandler {
                onTapped: point => examChart.hoverIndex = examChart.barAt(point.position.x)
            }
        }
        QQC2.Label {
            objectName: "examRoundsText"
            Layout.fillWidth: true
            text: page.rounds.length > 0
                  ? qsTr("Vertical: average of the ratings (1 %1 … 4 %2). Horizontal: date of the round.")
                        .arg(Practice.levelNames[1]).arg(Practice.levelNames[4])
                  : qsTr("No round in the exam mode yet.")
            color: KanteStyle.mutedTextColor
            font: KanteStyle.smallFont
            wrapMode: Text.Wrap
        }
    }
}
