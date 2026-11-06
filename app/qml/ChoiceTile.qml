import QtQuick
import QtQuick.Layouts
import Mitori

// A choice of the appearance page with its preview: the tile, below it a real sample (LookPreview).
// A tap on either applies the choice at once.
ColumnLayout {
    id: tile

    required property string text
    property string hint: ""
    required property bool checked
    required property int look
    required property var texts
    // The tile's name for the tests ("styleTile", "languageTile").
    property string tileName: ""
    signal chosen()

    spacing: Look.unit(4)

    LookOptionTile {
        objectName: tile.tileName
        Layout.fillWidth: true
        text: tile.text
        hint: tile.hint
        checked: tile.checked
        onClicked: tile.chosen()
    }
    LookPreview {
        objectName: "lookPreview"
        Layout.fillWidth: true
        Layout.preferredHeight: implicitHeight
        look: tile.look
        texts: tile.texts
        Accessible.ignored: true

        // Over the sample: it only shows.
        MouseArea {
            z: 1
            anchors.fill: parent
            hoverEnabled: true
            onClicked: tile.chosen()
        }
    }
}
