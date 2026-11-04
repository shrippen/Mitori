import QtQuick
import QtQuick.Controls as QQC2

// A scroll area: the style's ground, and a bar in the style's colours (the controls' own bar is
// the platform's, rounded and blue). The bar is anchored to the right edge, 4 px wide, in the
// style's rule colour.
QQC2.ScrollView {
    QQC2.ScrollBar.vertical: QQC2.ScrollBar {
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        policy: QQC2.ScrollBar.AsNeeded
        contentItem: Rectangle {
            implicitWidth: Look.unit(4)
            radius: width / 2
            color: Look.ruleColor
        }
    }
}
