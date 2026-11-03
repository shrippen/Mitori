import QtQuick
import org.kde.kirigami as Kirigami

// The window's head. Kante, Breeze and Material keep Kirigami's bar: nothing here. Under Shinki
// +shinki/LookHead.qml (ShinkiHead, bible section 5, Kopf) takes its place.
Item {
    property Kirigami.Page page: null
    property string backTitle: ""
    property string moreText: ""
    signal back()

    visible: false
    implicitHeight: 0
}
