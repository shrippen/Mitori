import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import Mitori

// Settings (clickdummy "Einstellungen"): the way to the videos, the backup and sync, and the
// appearance, each a page of its own.
//
//   Videos              Bundles, updates, code
//   Backup and sync     File, Nextcloud, other devices
//   Appearance          Style, language, about Mitori
Kirigami.ScrollablePage {
    id: page

    // ScrollablePage keeps the platform's colours (Theme.inherit: false): give it the style's, and
    // the style's page ground (Look: Kante or Shinki).
    property Item lookScope: LookScope { target: page }
    background: Rectangle { color: Look.backgroundColor }

    title: qsTr("Settings")
    // A new window (a change of style) comes back here (Main.keptPageList).
    readonly property string keptPage: "SettingsPage.qml"

    // ListAndPage, when it shows this page: on a tablet sideways the rows choose beside the list.
    property var paneHost: null
    readonly property var firstPane: ({ source: "VideosPage.qml", props: {} })
    function open(file) {
        if (!paneHost || !paneHost.show(file)) {
            page.Kirigami.PageStack.push(Qt.resolvedUrl(file))
        }
    }
    function shown(file) {
        return !!paneHost && paneHost.isShown(file)
    }

    ColumnLayout {
        spacing: 0

        LookListRow {
            objectName: "videosButton"
            Layout.fillWidth: true
            text: qsTr("Videos")
            subtitle: qsTr("Bundles, updates, code")
            selected: page.shown("VideosPage.qml")
            onClicked: page.open("VideosPage.qml")
        }
        LookListRow {
            objectName: "backupButton"
            Layout.fillWidth: true
            text: qsTr("Backup and sync")
            subtitle: qsTr("File, Nextcloud, other devices")
            selected: page.shown("BackupPage.qml")
            onClicked: page.open("BackupPage.qml")
        }
        LookListRow {
            objectName: "appearanceButton"
            Layout.fillWidth: true
            text: qsTr("Appearance")
            subtitle: qsTr("Style, language, about Mitori")
            selected: page.shown("AppearancePage.qml")
            onClicked: page.open("AppearancePage.qml")
        }
    }
}
