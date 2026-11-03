import QtQuick
import QtQuick.Controls.impl

// A theme icon in one colour (bible section 5: white on red and ink, red for a link). Kirigami's
// isMask colours only where the platform theme does it (KIconLoader on Plasma); Android, the
// AppImage and the tests drew the icon as it is (dark on red). The Controls' IconImage colours it
// on every platform, as the Controls styles do.
IconImage {
    property string source: ""

    name: source
    color: ShinkiStyle.ink
    sourceSize.width: ShinkiStyle.iconSize
    sourceSize.height: ShinkiStyle.iconSize
}
