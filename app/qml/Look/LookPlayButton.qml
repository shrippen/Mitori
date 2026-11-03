import QtQuick.Controls as QQC2
import Kante

// The play button of a list row: under Kante a symbol button, under Shinki the red circle
// (+shinki/LookPlayButton.qml). `text` names it for screen readers and the tooltip.
KanteToolButton {
    icon.name: "media-playback-start"
    display: QQC2.AbstractButton.IconOnly
}
