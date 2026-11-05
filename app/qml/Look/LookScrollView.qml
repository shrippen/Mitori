import QtQuick
import QtQuick.Controls as QQC2

// A scroll area: the style's ground, and a bar in the style's colours (the controls' own bar is
// the platform's, rounded and blue). The bar is anchored to the right edge, 4 px wide, in the
// style's rule colour.
QQC2.ScrollView {
    QQC2.ScrollBar.vertical: QQC2.ScrollBar {
        id: bar

        objectName: "lookScrollBar"
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        policy: QQC2.ScrollBar.AsNeeded
        padding: 0
        background: null
        contentItem: Rectangle {
            implicitWidth: Look.unit(4)
            // The style's own bar hides itself; this one replaces it and has to do so too.
            visible: bar.size < 1
            radius: width / 2
            color: Look.ruleColor
        }
    }
}
