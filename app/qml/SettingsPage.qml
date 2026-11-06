import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import Mitori

// Settings (clickdummy "Einstellungen"): the way to the videos, the backup and sync, and the
// appearance, each a page of its own.
//
//   映  Videos              Bundles, updates, code
//   保  Backup and sync     File, Nextcloud, other devices
//   色  Appearance          Style, language, about Mitori
Kirigami.ScrollablePage {
    id: page

    // ScrollablePage keeps the platform's colours (Theme.inherit: false): give it the style's, and
    // the style's page ground (Look: Kante or Shinki).
    property Item lookScope: LookScope { target: page }
    background: Rectangle { color: Look.backgroundColor }

    title: qsTr("Settings")
    // A new window (a change of style) comes back here (Main.keptPageList).
    readonly property string keptPage: "SettingsPage.qml"

    function open(file) {
        page.Kirigami.PageStack.push(Qt.resolvedUrl(file))
    }

    ColumnLayout {
        spacing: 0

        LookListRow {
            objectName: "videosButton"
            Layout.fillWidth: true
            leadingText: "映"
            text: qsTr("Videos")
            subtitle: qsTr("Bundles, updates, code")
            onClicked: page.open("VideosPage.qml")
        }
        LookListRow {
            objectName: "backupButton"
            Layout.fillWidth: true
            leadingText: "保"
            text: qsTr("Backup and sync")
            subtitle: qsTr("File, Nextcloud, other devices")
            onClicked: page.open("BackupPage.qml")
        }
        LookListRow {
            objectName: "appearanceButton"
            Layout.fillWidth: true
            leadingText: "色"
            text: qsTr("Appearance")
            subtitle: qsTr("Style, language, about Mitori")
            onClicked: page.open("AppearancePage.qml")
        }
    }
}
