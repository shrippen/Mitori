import QtQuick
import QtQuick.Controls as QQC2

// Figure (bible section 5, Kennzahl): the number 28 px bold in table figures, the word below as a
// column. `delta` the change ("+12 %"), `trend` 1 up, -1 down, 0 flat; `good` which way is good
// (1 up, -1 down, 0 none). Better is ink, worse red; the arrow is a shape, so colour never stands
// alone. Same interface as KanteKpi.
Column {
    id: kpi

    property string value: ""
    property string label: ""
    property string delta: ""
    property int trend: 0
    property int good: 1

    readonly property int sense: trend * good

    spacing: ShinkiStyle.gridSpace

    Accessible.role: Accessible.StaticText
    Accessible.name: label.length > 0 ? label + ": " + value : value
    Accessible.description: delta

    QQC2.Label {
        Accessible.ignored: true
        text: kpi.value
        color: ShinkiStyle.ink
        font.family: ShinkiStyle.uiFamily
        font.bold: true
        font.pixelSize: ShinkiStyle.kpiSize
        font.features: { "tnum": 1 }
    }
    QQC2.Label {
        Accessible.ignored: true
        text: kpi.label.toUpperCase()
        color: ShinkiStyle.inkMuted
        font.family: ShinkiStyle.narrowFamily
        font.bold: true
        font.pixelSize: ShinkiStyle.columnSize
        font.letterSpacing: ShinkiStyle.columnSize * ShinkiStyle.columnTracking
    }

    // The change: arrow (up, down, flat bar) and text, red when the news is bad.
    Row {
        id: change

        readonly property color tone: kpi.sense < 0 ? ShinkiStyle.red : ShinkiStyle.ink

        visible: kpi.delta.length > 0
        spacing: ShinkiStyle.gridSpace
        onToneChanged: arrow.requestPaint()

        Canvas {
            id: arrow

            width: ShinkiStyle.levelDotSize
            height: ShinkiStyle.levelDotSize
            anchors.verticalCenter: parent.verticalCenter
            onPaint: {
                const ctx = getContext("2d")
                ctx.reset()
                ctx.fillStyle = change.tone
                ctx.beginPath()
                if (kpi.trend > 0) {
                    ctx.moveTo(width / 2, 0); ctx.lineTo(width, height); ctx.lineTo(0, height)
                } else if (kpi.trend < 0) {
                    ctx.moveTo(0, 0); ctx.lineTo(width, 0); ctx.lineTo(width / 2, height)
                } else {
                    ctx.rect(0, height / 2 - ShinkiStyle.lineWidth, width, 2 * ShinkiStyle.lineWidth)
                }
                ctx.closePath()
                ctx.fill()
            }
            Component.onCompleted: requestPaint()
        }
        QQC2.Label {
            Accessible.ignored: true
            text: kpi.delta
            color: change.tone
            font.family: ShinkiStyle.uiFamily
            font.pixelSize: ShinkiStyle.smallSize
            font.features: { "tnum": 1 }
        }
    }
}
