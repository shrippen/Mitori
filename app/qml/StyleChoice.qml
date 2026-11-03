pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import Mitori

// The styles this device offers (Appearance.styles), one tile each; a tap chooses one for the
// next start. In the first-start wizard and in Settings → Appearance.
Flow {
    id: choice

    spacing: Look.unit(8)

    Repeater {
        model: Appearance.styles()

        LookOptionTile {
            required property var modelData

            objectName: "styleTile"
            width: Math.max(Look.unit(140), (choice.width - choice.spacing) / 2)
            text: modelData.name
            hint: modelData.text
            checked: Appearance.style === modelData.style
            onClicked: Appearance.setStyle(modelData.style)
        }
    }
}
