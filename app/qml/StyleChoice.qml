pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import Mitori

// The styles this device offers (Appearance.styles), one tile each with its preview, in the
// language chosen; a tap changes the style at once. In the first-start wizard and in
// Settings → Appearance.
Flow {
    id: choice

    spacing: Look.unit(8)

    Repeater {
        // The count: a new language changes the tiles' words, not the tiles (their samples stand).
        model: Appearance.styles.length

        ChoiceTile {
            required property int index
            readonly property var modelData: Appearance.styles[index]

            width: Math.max(Look.unit(140), (choice.width - choice.spacing) / 2)
            tileName: "styleTile"
            text: modelData.name
            hint: modelData.text
            checked: Appearance.style === modelData.style
            look: modelData.style
            texts: Appearance.sample(Appearance.language)
            onChosen: Appearance.setStyle(modelData.style)
        }
    }
}
