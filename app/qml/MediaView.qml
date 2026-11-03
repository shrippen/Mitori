import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import QtMultimedia
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

    QQC2.Label {
        objectName: "mediaMissing"
        Layout.fillWidth: true
        visible: !view.here
        text: view.kind === Media.Kind.Photo ? qsTr("The photo is not on this device.") : qsTr("The voice note is not on this device.")
        color: Look.mutedTextColor
        font: Look.smallFont
        wrapMode: Text.Wrap
    }

    RowLayout {
        visible: view.here && view.kind === Media.Kind.Voice
        spacing: Kirigami.Units.smallSpacing

        LookToolButton {
            objectName: "playVoice"
            icon.name: player.playing ? "media-playback-stop" : "media-playback-start"
            text: player.playing ? qsTr("Stop") : qsTr("Play voice note")
            display: QQC2.AbstractButton.IconOnly
            onClicked: {
                if (player.playing) {
                    player.stop()
                    return
                }
                player.source = Media.url(view.media)    // loaded on the first tap, not with the list
                player.play()
            }
        }
        QQC2.Label {
            objectName: "voiceLength"
            text: player.duration > 0 ? qsTr("Voice note %1").arg(view.clock(player.playing ? player.position : player.duration))
                                      : qsTr("Voice note")
            color: Look.mutedTextColor
        }
    }

    // "0:14"
    function clock(ms) {
        const s = Math.round(ms / 1000)
        return Math.floor(s / 60) + ":" + String(s % 60).padStart(2, "0")
    }

    MediaPlayer {
        id: player

        objectName: "voicePlayer"
        audioOutput: AudioOutput {}
    }
}
