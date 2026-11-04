import QtQuick
import QtQuick.Controls as QQC2

// A field for a longer text (a note): the style's ink on the style's ground.
QQC2.TextArea {
    color: Look.textColor
    font: Look.readingFont
    placeholderTextColor: Look.mutedTextColor
    selectByMouse: true
}
