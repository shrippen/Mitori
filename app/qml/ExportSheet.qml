import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import Mitori

// What goes out when a notebook is exported (clickdummy "Exportieren"): the archive's name, how
// many photos and voice notes, its size and the text of notizbuch.md; then Share (only where the
// system has a share dialog, Courses.canShare) and Save. The form as TechniqueSheet: from the
// bottom under Shinki, a centred dialog otherwise.
Kirigami.Dialog {
    id: sheet

    property string uid: ""
    // Courses.exportPreview, read on opening.
    property var preview: ({})
    property bool shareable: Courses.canShare
    readonly property Flickable flickable: ((contentItem as QQC2.ScrollView)?.contentItem as Flickable) ?? null
    readonly property bool shinkiSheet: Look.shinki

    signal saveRequested()

    function show() {
        sheet.preview = Courses.exportPreview(sheet.uid)
        sheet.open()
    }

    title: sheet.preview.title || ""
    preferredWidth: Kirigami.Units.gridUnit * 24
    width: shinkiSheet && parent ? parent.width : Math.min(preferredWidth, absoluteMaximumWidth)
    height: shinkiSheet && flickable ? Math.min(flickable.contentHeight + topPadding + bottomPadding + 2 * Look.borderWidth,
                                                maximumHeight)
                                     : implicitHeight
    y: parent ? (shinkiSheet ? Math.round(parent.height - height) : Math.round((parent.height - height) / 2)) : 0
    padding: shinkiSheet ? Look.unit(16) : Kirigami.Units.largeSpacing

    background: Rectangle {
        color: Look.backgroundColor
        border.width: Look.borderWidth
        border.color: Look.ruleColor
    }

    property Item lookScope: LookScope { target: sheet.contentItem }
    property Item headScope: LookScope { target: sheet.header }

    // Keys scroll a text taller than the window, as in TechniqueSheet.
    function scrollKey(event) {
        const f = sheet.flickable
        const line = Kirigami.Units.gridUnit * 2
        const step = {
            [Qt.Key_Up]: -line, [Qt.Key_Down]: line,
            [Qt.Key_PageUp]: f ? -f.height * 0.9 : 0, [Qt.Key_PageDown]: f ? f.height * 0.9 : 0,
            [Qt.Key_Home]: -Infinity, [Qt.Key_End]: Infinity
        }[event.key]
        if (!f || step === undefined) {
            return
        }
        f.contentY = Math.max(0, Math.min(Math.max(0, f.contentHeight - f.height), f.contentY + step))
        event.accepted = true
    }

    header: Kirigami.DialogHeader {
        dialog: sheet
        visible: !sheet.shinkiSheet
        height: sheet.shinkiSheet ? 0 : implicitHeight
        contentItem: Kirigami.DialogHeaderTopContent {
            dialog: sheet
            showCloseButton: false

            LookToolButton {
                icon.name: "window-close-symbolic"
                text: qsTr("Close")
                display: QQC2.AbstractButton.IconOnly
                onClicked: sheet.close()
            }
        }
        Keys.onPressed: event => sheet.scrollKey(event)
    }

    onOpened: column.forceActiveFocus()

    ColumnLayout {
        id: column

        spacing: Kirigami.Units.smallSpacing
        Keys.onPressed: event => sheet.scrollKey(event)

        // The head under Shinki: the grip, the title in the book face.
        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: sheetHead.implicitHeight + Look.unit(16)
            visible: sheet.shinkiSheet

            Rectangle {
                anchors.top: parent.top
                anchors.horizontalCenter: parent.horizontalCenter
                width: Look.unit(40)
                height: Look.unit(4)
                color: Look.ruleColor
            }

            LookLabel {
                id: sheetHead

                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.bottomMargin: Look.unit(8)
                text: sheet.title
                font: Look.sheetTitleFont
                wrapMode: Text.Wrap
            }

            LookToolButton {
                anchors.top: parent.top
                anchors.right: parent.right
                icon.name: "window-close"
                text: qsTr("Close")
                display: QQC2.AbstractButton.IconOnly
                onClicked: sheet.close()
            }
        }

        LookLabel {
            objectName: "exportFile"
            Layout.fillWidth: true
            text: sheet.preview.name || ""
            color: Look.mutedTextColor
            wrapMode: Text.WrapAtWordBoundaryOrAnywhere
        }
        LookLabel {
            objectName: "exportCounts"
            Layout.fillWidth: true
            text: [qsTr("%n photo(s)", "", sheet.preview.photos || 0),
                   qsTr("%n voice note(s)", "", sheet.preview.voices || 0),
                   sheet.preview.size || ""].join(" · ")
            wrapMode: Text.Wrap
        }
        LookLabel {
            objectName: "exportMissing"
            Layout.fillWidth: true
            visible: (sheet.preview.missing || 0) > 0
            text: qsTr("%n file(s) are not on this device and stay out.", "", sheet.preview.missing || 0)
            color: Look.neutralTextColor
            wrapMode: Text.Wrap
        }

        // Before the text, so they stay in view with a long notebook. Wraps on a small phone.
        Flow {
            Layout.fillWidth: true
            Layout.topMargin: Kirigami.Units.largeSpacing
            spacing: Kirigami.Units.smallSpacing

            LookButton {
                objectName: "exportShare"
                visible: sheet.shareable
                role: LookButton.Role.Primary
                icon.name: "document-share"
                text: qsTr("Share …")
                onClicked: {
                    Courses.shareNotebook(sheet.uid)    // a failure shows on the page
                    sheet.close()
                }
            }
            LookButton {
                objectName: "exportSave"
                role: sheet.shareable ? LookButton.Role.Normal : LookButton.Role.Primary
                icon.name: "document-save"
                text: qsTr("Save")
                onClicked: {
                    sheet.close()
                    sheet.saveRequested()
                }
            }
        }
        LookLabel {
            objectName: "exportAbout"
            Layout.fillWidth: true
            text: qsTr("A small archive: Markdown, photos and voice notes.")
            color: Look.mutedTextColor
            font: Look.smallFont
            wrapMode: Text.Wrap
        }

        // The file as it goes out; what the user wrote is text, never markup.
        Rectangle {
            Layout.fillWidth: true
            Layout.topMargin: Kirigami.Units.largeSpacing
            implicitHeight: exportText.implicitHeight + 2 * Kirigami.Units.smallSpacing
            color: Look.chipColor

            LookLabel {
                id: exportText

                objectName: "exportText"
                anchors.fill: parent
                anchors.margins: Kirigami.Units.smallSpacing
                text: sheet.preview.text || ""
                textFormat: Text.PlainText
                font: Look.smallFont
                wrapMode: Text.WrapAtWordBoundaryOrAnywhere
            }
        }
    }
}
