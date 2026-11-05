import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import Mitori

// A note's photo or voice note (Media): the photo fits the width; a voice note plays on a tap,
// its length shown once it is loaded. Nothing for a note without one; a file that is not on
// the device (restored from a cloud backup, which leaves the media out) says so.
ColumnLayout {
    id: view

    required property string media
    readonly property int kind: Media.kind(media)
    readonly property bool here: kind !== Media.Kind.None && Media.exists(media)

    visible: kind !== Media.Kind.None
    spacing: Kirigami.Units.smallSpacing

    Image {
        objectName: "notePhoto"
        Layout.fillWidth: true
        Layout.maximumHeight: Kirigami.Units.gridUnit * 14
        visible: view.here && view.kind === Media.Kind.Photo
        source: visible ? Media.url(view.media) : ""
        sourceSize.width: 1024              // a phone's width is enough; the file has up to 2048
        fillMode: Image.PreserveAspectFit
        horizontalAlignment: Image.AlignLeft
        asynchronous: true
        Accessible.role: Accessible.Graphic
        Accessible.name: qsTr("Photo")
    }

    LookLabel {
        objectName: "mediaMissing"
        Layout.fillWidth: true
        visible: !view.here
        text: view.kind === Media.Kind.Photo ? qsTr("The photo is not on this device.") : qsTr("The voice note is not on this device.")
        color: Look.mutedTextColor
        font: Look.smallFont
        wrapMode: Text.Wrap
    }

    // Its own file: QtMultimedia takes a second to load, and most lists of notes have no voice.
    Loader {
        active: view.here && view.kind === Media.Kind.Voice
        visible: active
        source: "MediaVoice.qml"
        onLoaded: item.media = view.media
    }
}
