import QtQuick
import QtQuick.Controls as QQC2

// A menu (the entries: LookMenuItem): the style's ground with a frame of its own.
QQC2.Menu {
    background: Rectangle {
        color: Look.backgroundColor
        border.width: Look.borderWidth
        border.color: Look.ruleColor
    }
}
