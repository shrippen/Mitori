import QtQuick
import QtQuick.Controls as QQC2
import "."

/**
 * Days on a calendar grid (Kante 1.62, for Mitori's practice days): a column per week, a row per
 * weekday, the newest week on the right. The axes are labelled: weekday names on every second row
 * at the side, the month's name above the week it begins in (the first column names its month
 * when the next name is three weeks away or more). Each day has a level 0..4, coloured as KanteHeatmap
 * does (KanteStyle.levelColor); hovering a day, or tapping it (for a few seconds, on touch), shows
 * its date and `tips` entry; screen readers hear the date in full. Mirrored right to left with
 * LayoutMirroring (the newest week on the left). Pair it with a KanteLegend for the levels.
 *   endDate         the last day shown (today unless set); later days of its week stay empty
 *   weeks           how many weeks (columns)
 *   levels          {"2026-11-02": 3, …} by ISO day (isoDay()); a missing day is 0, above 4 is 4
 *   tips            {"2026-11-02": "5 techniques", …}: shown after the date
 *   firstDayOfWeek  Locale.Monday …, the locale's unless set
 *   locale          for day and month names and dates
 */
Item {
    id: heat

    property date endDate: new Date()
    property int weeks: 26
    property var levels: ({})
    property var tips: ({})
    property var locale: Qt.locale()
    property int firstDayOfWeek: locale.firstDayOfWeek
    property int cellSize: KanteStyle.unit(12)
    property int spacing: 2

    readonly property real step: cellSize + spacing
    readonly property real labelWidth: Math.ceil(Math.max(...weekdayLabels().map(t => monthMetrics.advanceWidth(t)))) + KanteStyle.unit(6)
    readonly property real labelHeight: monthMetrics.height + KanteStyle.unit(2)
    readonly property bool mirrored: LayoutMirroring.enabled
    // The day tapped last, its tooltip shown for a few seconds (touch has no hover).
    property int tappedIndex: -1

    Accessible.role: Accessible.Grouping

    // x of an item `w` wide at `x` from the leading edge; mirrored at the width it gets (1.62.1).
    function lead(x, w) {
        return mirrored ? width - x - w : x
    }

    implicitWidth: labelWidth + weeks * step - spacing
    implicitHeight: labelHeight + 7 * step - spacing

    // Local noon, so that adding days never crosses midnight on a change of summer time.
    function noon(d) {
        return new Date(d.getFullYear(), d.getMonth(), d.getDate(), 12)
    }
    function isoDay(d) {
        const p = n => (n < 10 ? "0" : "") + n
        return d.getFullYear() + "-" + p(d.getMonth() + 1) + "-" + p(d.getDate())
    }
    readonly property date lastDay: noon(endDate)
    // The first day of the first column.
    readonly property date firstDay: {
        const d = noon(endDate)
        const back = (d.getDay() - firstDayOfWeek + 7) % 7 + (weeks - 1) * 7
        return new Date(d.getFullYear(), d.getMonth(), d.getDate() - back, 12)
    }
    function dayAt(week, row) {
        return new Date(firstDay.getFullYear(), firstDay.getMonth(), firstDay.getDate() + week * 7 + row, 12)
    }
    function shown(week, row) {
        return dayAt(week, row) <= lastDay
    }
    function levelOf(day) {
        return Math.max(0, Math.min(4, Number(levels[isoDay(day)]) || 0))
    }
    function tipOf(day, format) {
        const date = day.toLocaleDateString(locale, format === undefined ? Locale.ShortFormat : format)
        const tip = tips[isoDay(day)]
        return tip ? date + " · " + tip : date
    }
    // Names on rows 1, 3 and 5 (Tue, Thu, Sat when weeks start on Monday), "" elsewhere.
    function weekdayLabels() {
        const rows = []
        for (let r = 0; r < 7; r++) {
            rows.push(r % 2 === 1 ? locale.dayName((firstDayOfWeek + r) % 7, Locale.ShortFormat) : "")
        }
        return rows
    }
    // [{week, month, text}]: the weeks a month begins in; the first column names the month of its
    // last drawn day when the next name is at least three weeks away.
    function monthLabels() {
        const result = []
        const label = (w, m) => ({ week: w, month: m, text: locale.standaloneMonthName(m, Locale.ShortFormat) })
        for (let w = 0; w < weeks; w++) {
            for (let r = 0; r < 7; r++) {
                const d = dayAt(w, r)
                if (d.getDate() === 1 && d <= lastDay) {
                    result.push(label(w, d.getMonth()))
                }
            }
        }
        if (result.length === 0 || result[0].week >= 3) {
            const last = dayAt(0, 6) > lastDay ? lastDay : dayAt(0, 6)     // one week: the last drawn day
            result.unshift(label(0, last.getMonth()))
        }
        return result
    }

    FontMetrics { id: monthMetrics; font: KanteStyle.smallFont }

    Repeater {
        model: heat.weekdayLabels()
        delegate: Text {
            required property int index
            required property string modelData
            x: heat.lead(0, implicitWidth)
            y: heat.labelHeight + index * heat.step + (heat.cellSize - height) / 2
            text: modelData
            font: KanteStyle.smallFont
            color: KanteStyle.mutedTextColor
        }
    }

    Repeater {
        model: heat.monthLabels()
        delegate: Text {
            required property var modelData
            // Kept inside the grid when a month begins in one of the last weeks.
            x: heat.lead(Math.min(heat.labelWidth + modelData.week * heat.step, heat.implicitWidth - implicitWidth), implicitWidth)
            y: 0
            text: modelData.text
            font: KanteStyle.smallFont
            color: KanteStyle.mutedTextColor
        }
    }

    Repeater {
        model: heat.weeks * 7
        delegate: Item {
            id: cell
            required property int index
            readonly property int week: Math.floor(index / 7)
            readonly property int row: index % 7
            readonly property date day: heat.dayAt(week, row)
            readonly property int level: heat.levelOf(day)
            readonly property string tip: heat.tipOf(day)

            visible: heat.shown(week, row)
            x: heat.lead(heat.labelWidth + week * heat.step, heat.cellSize)
            y: heat.labelHeight + row * heat.step
            width: heat.cellSize
            height: heat.cellSize
            Accessible.role: Accessible.StaticText
            Accessible.name: heat.tipOf(day, Locale.LongFormat)

            Rectangle { anchors.fill: parent; color: KanteStyle.sunkenColor }
            Rectangle { anchors.fill: parent; color: cell.level > 0 ? KanteStyle.levelColor(cell.level, 4) : "transparent" }

            HoverHandler { id: hover }
            TapHandler {
                onTapped: {
                    heat.tappedIndex = cell.index
                    tipTimer.restart()
                }
            }
            QQC2.ToolTip.text: tip
            QQC2.ToolTip.visible: hover.hovered || heat.tappedIndex === index
            QQC2.ToolTip.delay: heat.tappedIndex === index ? 0 : 300
        }
    }

    Timer {
        id: tipTimer
        interval: 3000
        onTriggered: heat.tappedIndex = -1
    }
}
