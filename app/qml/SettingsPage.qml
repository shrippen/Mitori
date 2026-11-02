import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import Kante
import Mitori

// Settings: the video bundles (codes of the DVDs) and a folder with exam videos of one's own.
Kirigami.ScrollablePage {
    id: page

    // ScrollablePage keeps the platform's colours (Theme.inherit: false): give it Kante's, and
    // Kante's page ground (the scope's background role is the dialog colour).
    property Item kanteScope: KanteScope { target: page }
    background: Rectangle { color: KanteStyle.backgroundColor }

    title: qsTr("Settings")

    Kirigami.FormLayout {
        KanteButton {
            objectName: "videosButton"
            Kirigami.FormData.label: qsTr("Exam videos:")
            icon.name: "media-playback-start"
            text: qsTr("Videos and DVD codes")
            onClicked: page.Kirigami.PageStack.push(Qt.resolvedUrl("VideosPage.qml"))
        }
        KanteTextField {
            id: folderField

            Kirigami.FormData.label: qsTr("Video folder:")
            Layout.fillWidth: true
            text: Videos.folder
            onEditingFinished: Videos.folder = text
        }
        QQC2.Label {
            Layout.fillWidth: true
            Layout.maximumWidth: Kirigami.Units.gridUnit * 30
            wrapMode: Text.Wrap
            color: KanteStyle.disabledTextColor
            text: qsTr("Without a bundle: a folder with the videos, folders and names as on the DVDs: Aikido/5. Kyu.mkv, Daitoryu/1. Dan.mkv, Shinki Toho/Shoden.mkv …")
                  + (Videos.sandboxed ? " " + qsTr("The Flatpak can read folders under ~/Videos only.") : "")
        }
    }
}
