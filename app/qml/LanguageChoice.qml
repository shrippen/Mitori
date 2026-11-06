pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import Mitori

// The languages (in the order of Appearance.Language), one tile each with a preview in the style
// that runs and in that language; a tap changes the language at once, every page stays.
Flow {
    id: choice

    spacing: Look.unit(8)

    readonly property var names: [qsTr("System"), "Deutsch", "English"]

    Repeater {
        // The count: a new language changes a tile's word, not the tile (its sample stands).
        model: choice.names.length

        ChoiceTile {
            required property int index

            width: Math.max(Look.unit(140), (choice.width - choice.spacing) / 2)
            tileName: "languageTile"
            text: choice.names[index]
            checked: Appearance.language === index
            look: Appearance.running
            texts: Appearance.sample(index)
            onChosen: Appearance.setLanguage(index)
        }
    }
}
