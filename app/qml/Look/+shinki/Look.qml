pragma Singleton
import QtQuick
import Shinki

// The roles of Look.qml under Shinki (shinki/README.md): the same names, from ShinkiStyle.
// Shinki has one colour: red is the accent, errors are red with a word, success is ink.
QtObject {
    readonly property bool shinki: true

    readonly property color backgroundColor: ShinkiStyle.paper
    readonly property color textColor: ShinkiStyle.ink
    readonly property color mutedTextColor: ShinkiStyle.inkMuted
    readonly property color disabledTextColor: ShinkiStyle.inkMuted
    readonly property color accentColor: ShinkiStyle.red
    readonly property color negativeTextColor: ShinkiStyle.red
    readonly property color positiveTextColor: ShinkiStyle.ink
    readonly property color neutralTextColor: ShinkiStyle.inkMuted
    readonly property color cardColor: ShinkiStyle.paperAlt
    readonly property color ruleColor: ShinkiStyle.line
    readonly property color stageColor: ShinkiStyle.video
    readonly property color stageTextColor: ShinkiStyle.videoText
    readonly property int borderWidth: ShinkiStyle.lineWidth

    readonly property font defaultFont: Qt.font({ family: ShinkiStyle.uiFamily, pixelSize: ShinkiStyle.uiSize })
    readonly property font smallFont: Qt.font({ family: ShinkiStyle.uiFamily, pixelSize: ShinkiStyle.smallSize })

    function unit(px) {
        return ShinkiStyle.unit(px)
    }
    function levelColor(level) {
        return ShinkiStyle.levelColor(level)
    }
    // Titles in the book face (Crimson Pro), as the sheet and player titles of the bible; without
    // a size the player title's.
    function titleFont(pointSize) {
        return pointSize > 0 ? Qt.font({ family: ShinkiStyle.bookFamily, weight: Font.DemiBold, pointSize: pointSize })
                             : Qt.font({ family: ShinkiStyle.bookFamily, weight: Font.DemiBold, pixelSize: ShinkiStyle.playerTitleSize })
    }
    // A font in the Mincho, its size as the base has it (points or pixels, never both).
    function japaneseFont(base) {
        const font = base ? base : defaultFont
        return font.pointSize > 0 ? Qt.font({ family: ShinkiStyle.japaneseFamily, pointSize: font.pointSize, weight: font.weight })
                                  : Qt.font({ family: ShinkiStyle.japaneseFamily, pixelSize: font.pixelSize, weight: font.weight })
    }
}
