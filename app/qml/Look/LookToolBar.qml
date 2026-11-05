import QtQuick
import QtQuick.Controls as QQC2

// A bar of actions at the foot of a page: the style's ground, the actions as the page puts them
// (the buttons themselves are the style's).
QQC2.ToolBar {
    // All four: Material sets each side itself (the safe area), so `padding` would not reach them.
    leftPadding: Look.unit(8)
    rightPadding: Look.unit(8)
    topPadding: Look.unit(8)
    bottomPadding: Look.unit(8)

    background: Rectangle {
        color: Look.backgroundColor
    }
}
