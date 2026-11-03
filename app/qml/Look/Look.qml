pragma Singleton
import QtQuick
import Kante

// The roles a page reads (shinki/README.md, section 6), here from Kante: Breeze, Material, Kante
// and Leinen. Under Shinki the file selector takes +shinki/Look.qml with the same names. A page
// names the role (Look.mutedTextColor), never KanteStyle or ShinkiStyle.
QtObject {
    readonly property bool shinki: false

    readonly property color backgroundColor: KanteStyle.backgroundColor
    readonly property color textColor: KanteStyle.textColor
    readonly property color mutedTextColor: KanteStyle.mutedTextColor
    readonly property color disabledTextColor: KanteStyle.disabledTextColor
    readonly property color accentColor: KanteStyle.focusColor
    readonly property color negativeTextColor: KanteStyle.negativeTextColor
    readonly property color positiveTextColor: KanteStyle.positiveTextColor
    readonly property color neutralTextColor: KanteStyle.neutralTextColor
    readonly property color cardColor: KanteStyle.cardColor
    readonly property color ruleColor: KanteStyle.ruleColor
    readonly property color stageColor: KanteStyle.stageColor
    readonly property color stageTextColor: KanteStyle.stageTextColor
    readonly property int borderWidth: KanteStyle.borderWidth
    // The keyboard focus frame.
    readonly property color focusColor: KanteStyle.focusColor
    readonly property int focusWidth: KanteStyle.borderWidth

    readonly property font defaultFont: KanteStyle.defaultFont
    readonly property font smallFont: KanteStyle.smallFont

    function unit(px) {
        return KanteStyle.unit(px)
    }
    // Level 0..4 (open … exam-ready).
    function levelColor(level) {
        return KanteStyle.levelColor(level)
    }
    // Figures that line up (times, counts).
    function monoFont(pointSize) {
        return KanteStyle.monoFont(pointSize)
    }
    // Large display text (a mock exam's technique).
    function headingFont(pointSize) {
        return KanteStyle.headingFont(pointSize)
    }
    function titleFont(pointSize) {
        return KanteStyle.titleFont(pointSize)
    }
    function japaneseFont(base) {
        return KanteStyle.japaneseFont(base)
    }
}
