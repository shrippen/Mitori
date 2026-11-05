import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import QtMultimedia
import org.kde.kirigami as Kirigami
import Mitori

// A note's voice note (MediaView): plays on a tap, its length shown once it is loaded.
RowLayout {
    id: voice

    property string media
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
            player.source = Media.url(voice.media)    // loaded on the first tap, not with the list
            player.play()
        }
    }
    LookLabel {
        objectName: "voiceLength"
        text: player.duration > 0 ? qsTr("Voice note %1").arg(voice.clock(player.playing ? player.position : player.duration))
                                  : qsTr("Voice note")
        color: Look.mutedTextColor
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
