import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import Kante
import Mitori

// A style's sample in the appearance page (LookPreview): a title, a text, a chip and a button on a
// card, by the Look components of the engine it runs in. It only shows: a tap goes to its tile.
// No service here: this engine is not the app's, its texts come from the page.
Item {
    id: sample

    required property var texts
    // KanteStyle as in Main.qml. Without both the platform's colours, or Shinki (Look.shinki).
    required property bool kanteDark
    required property bool kanteLight

    implicitHeight: content.implicitHeight + 2 * content.anchors.margins

    Binding {
        target: KanteStyle
        property: "kind"
        value: sample.kanteDark || sample.kanteLight ? KanteStyle.Kind.Kante : KanteStyle.Kind.System
    }
    Binding {
        target: KanteStyle
        property: "preferDark"
        value: sample.kanteDark
    }
    Binding {
        target: KanteStyle
        property: "preferLight"
        value: sample.kanteLight
    }

    // Its own colours, not those of the page around it.
    Kirigami.Theme.inherit: false
    property Item lookScope: LookScope { target: sample }

    Rectangle {
        anchors.fill: parent
        color: Look.backgroundColor
        border.color: Look.ruleColor
        border.width: Look.borderWidth
    }
    LookCard {
        anchors.fill: content
        anchors.margins: -Look.unit(6)
    }
    ColumnLayout {
        id: content

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: Look.unit(12)
        spacing: Look.unit(6)

        LookHeading {
            objectName: "sampleTitle"
            Layout.fillWidth: true
            level: 3
            wrapMode: Text.Wrap
            text: sample.texts.title ?? ""
        }
        LookLabel {
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            font: Look.smallFont
            text: sample.texts.text ?? ""
        }
        Flow {
            Layout.fillWidth: true
            spacing: Look.unit(6)

            LookChip {
                objectName: "sampleChip"
                interactive: false
                text: sample.texts.chip ?? ""
            }
            LookButton {
                objectName: "sampleButton"
                focusPolicy: Qt.NoFocus
                role: LookButton.Role.Primary
                text: sample.texts.button ?? ""
            }
        }
    }
}
