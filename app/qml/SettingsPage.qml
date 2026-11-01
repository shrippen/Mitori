import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import Kante
import Mitori

// Settings: for now the folder with the exam videos (until the video bundles, ROADMAP.md step 4).
Kirigami.ScrollablePage {
    id: page

    title: qsTr("Settings")

    Kirigami.FormLayout {
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
            text: qsTr("Folders and names as on the DVDs: Aikido/5. Kyu.mkv, Daitoryu/1. Dan.mkv, Shinki Toho/Shoden.mkv … The Flatpak can read folders under ~/Videos only.")
        }
    }
}
