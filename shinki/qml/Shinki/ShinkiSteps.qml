pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts

// Steps of a wizard (bible section 5, Schritte): equal columns, each a bar on top, a square with
// the number and the label. Every state has a shape besides its colour:
//   done      bar in ink, the square filled ink with a drawn check
//   current   bar 3 px red, the number white on a red square, the label bold
//   upcoming  bar 1 px line, the number in a hollow square, the label inkMuted
//   skipped   a passed step in `skipped`: bar in line, a dash in a hollow square
// Same interface as KanteSteps (model, current, stepText, stateNames, skipped).
Item {
    id: steps

    property var model: []
    property int current: 0
    property string stepText: "Schritt %1 von %2"
    property var stateNames: ({ done: "erledigt", current: "aktuell", upcoming: "offen", skipped: "übersprungen" })
    property var skipped: []
    readonly property alias count: repeater.count

    function stateOf(i) {
        return i === current ? "current" : i > current ? "upcoming" : skipped.indexOf(i) >= 0 ? "skipped" : "done"
    }

    implicitWidth: row.implicitWidth
    implicitHeight: row.implicitHeight

    RowLayout {
        id: row

        anchors.fill: parent
        spacing: ShinkiStyle.gridSpace

        Repeater {
            id: repeater

            model: steps.model

            ColumnLayout {
                id: step

                required property int index
                required property var modelData
                readonly property string state: steps.stateOf(index)

                Layout.fillWidth: true
                Layout.preferredWidth: 1
                spacing: ShinkiStyle.gridSpace * 2

                Accessible.role: Accessible.StaticText
                Accessible.name: steps.stepText.arg(index + 1).arg(steps.count) + ": " + modelData + ", "
                                 + steps.stateNames[state]

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: step.state === "current" ? ShinkiStyle.selectedBarWidth : ShinkiStyle.lineWidth
                    color: step.state === "current" ? ShinkiStyle.red : step.state === "done" ? ShinkiStyle.ink : ShinkiStyle.line
                }
                RowLayout {
                    spacing: ShinkiStyle.gridSpace * 2

                    Rectangle {
                        Layout.preferredWidth: ShinkiStyle.iconSize + ShinkiStyle.gridSpace
                        Layout.preferredHeight: Layout.preferredWidth
                        color: step.state === "current" ? ShinkiStyle.red : step.state === "done" ? ShinkiStyle.ink : ShinkiStyle.paper
                        border.width: step.state === "upcoming" || step.state === "skipped" ? ShinkiStyle.lineWidth : 0
                        border.color: ShinkiStyle.line

                        QQC2.Label {
                            anchors.centerIn: parent
                            visible: step.state !== "done"
                            text: step.state === "skipped" ? "–" : String(step.index + 1)
                            color: step.state === "current" ? ShinkiStyle.textOnRed : ShinkiStyle.inkMuted
                            font.family: ShinkiStyle.uiFamily
                            font.bold: true
                            font.pixelSize: ShinkiStyle.smallSize
                            font.features: { "tnum": 1 }
                        }
                        // The check: two strokes in white.
                        Canvas {
                            anchors.fill: parent
                            anchors.margins: ShinkiStyle.gridSpace
                            visible: step.state === "done"
                            onPaint: {
                                const ctx = getContext("2d")
                                ctx.reset()
                                ctx.strokeStyle = ShinkiStyle.textOnRed
                                ctx.lineWidth = 2 * ShinkiStyle.lineWidth
                                ctx.beginPath()
                                ctx.moveTo(width * 0.15, height * 0.55)
                                ctx.lineTo(width * 0.4, height * 0.8)
                                ctx.lineTo(width * 0.85, height * 0.25)
                                ctx.stroke()
                            }
                        }
                    }
                    QQC2.Label {
                        Layout.fillWidth: true
                        text: step.modelData
                        color: step.state === "current" ? ShinkiStyle.ink : ShinkiStyle.inkMuted
                        font.family: ShinkiStyle.uiFamily
                        font.bold: step.state === "current"
                        font.pixelSize: ShinkiStyle.uiSize
                        elide: Text.ElideRight
                    }
                }
            }
        }
    }
}
