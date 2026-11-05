import QtQuick
import QtQuick.Controls.impl

/**
 * A theme icon in one colour (1.73). Kirigami.Icon's isMask colours only where the platform theme
 * does it (Plasma); on Android, in an AppImage and in tests the icon stayed as it is, dark on the
 * dark Kante ground. The Controls' IconImage colours it on every platform.
 *   iconSource  a theme name ("go-next") or a url
 *   color       the colour of the symbol
 */
IconImage {
    property var iconSource: ""
    readonly property bool isUrl: /^[a-z]+:|^\//.test(String(iconSource))

    name: isUrl ? "" : String(iconSource)
    source: isUrl ? iconSource : ""
    sourceSize.width: width
    sourceSize.height: height
}
