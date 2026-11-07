pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

// Head of the window (bible section 5, Kopf), in place of Kirigami's bar: above, the way back as
// a red column ("‹ PROGRAMME", the page before); below, the page's title in PT Sans bold, its
// Japanese small in inkMuted ("Mitori 見取り"); on the right the page's actions as symbol buttons,
// those Kirigami hides (DisplayHint.AlwaysHide) in a menu. A line closes it below.
//
//   ‹ PROGRAMME
//   Notes                              [▦] [⋮]
//   ──────────────────────────────────────────
//
//   page         the page shown (its title, actions; `headless: true` hides the head)
//   backTitle    the title of the page before, "" on the first
//   moreText     the name of the menu with the hidden actions ("More")
//   back()       the way back was taken
//
// Its height stays the same on every page but the start menu, which has no way back (the way back
// keeps its line, the title row the height of a button): Kirigami stops a page's slide when the
// view's height changes, so that slide ends at once (Main.dropLeaving takes the page that leaves).
Rectangle {
    id: head

    property Kirigami.Page page: null
    property string backTitle: ""
    property string moreText: ""
    // The mark of the app (the start menu has one, the other pages the way back instead).
    property string logo: ""
    signal back()

    // A title split at its first Japanese character: "Mitori 見取り" → "Mitori", "見取り".
    readonly property var parts: split(page && page.title ? String(page.title) : "")
    readonly property string title: parts[0]
    readonly property string kanji: parts[1]

    function split(text) {
        const at = text.search(/[\u3000-\u9fff\uff00-\uffef]/)
        if (at < 0) {
            return [text, ""]
        }
        return [text.slice(0, at).trim(), text.slice(at).trim()]
    }

    readonly property var actions: page && page.actions ? Array.from(page.actions).filter(a => a && a.visible !== false) : []
    readonly property var shown: actions.filter(a => !(a.displayHint & Kirigami.DisplayHint.AlwaysHide))
    readonly property var hidden: actions.filter(a => a.displayHint & Kirigami.DisplayHint.AlwaysHide)

    // A page that asks for no head (the player held sideways) gets none.
    visible: !!page && !own(page, "headless")

    // A page's own flag, which Kirigami.Page does not know (PlayerPage.headless).
    function own(object, name) {
        return object[name] === true
    }

    implicitHeight: visible ? column.implicitHeight + 2 * ShinkiStyle.gridSpace * 2 : 0
    color: ShinkiStyle.paper

    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: ShinkiStyle.lineWidth
        color: ShinkiStyle.line
    }

    ColumnLayout {
        id: column

        anchors.fill: parent
        anchors.leftMargin: ShinkiStyle.headSpace
        anchors.rightMargin: ShinkiStyle.gridSpace * 2
        anchors.topMargin: ShinkiStyle.gridSpace * 2
        anchors.bottomMargin: ShinkiStyle.gridSpace * 2
        spacing: 0

        // The heights of the two lines from their fonts, not their texts (a fallback font for a
        // rare glyph is taller).
        FontMetrics {
            id: backMetrics
            font.family: ShinkiStyle.narrowFamily
            font.bold: true
            font.pixelSize: ShinkiStyle.columnSize
        }
        FontMetrics {
            id: titleMetrics
            font.family: ShinkiStyle.uiFamily
            font.bold: true
            font.pixelSize: ShinkiStyle.pageTitleSize
        }

        // The way back: a column in red, its own button.
        QQC2.AbstractButton {
            id: backButton

            objectName: "headBack"
            enabled: head.backTitle !== "" && head.logo === ""
            visible: head.logo === ""   // the start menu has its mark in the title's row
            opacity: enabled ? 1 : 0
            Accessible.ignored: !enabled
            text: "‹ " + head.split(head.backTitle)[0].toUpperCase()
            Accessible.name: head.backTitle
            onClicked: head.back()
            hoverEnabled: true
            padding: 0
            topPadding: ShinkiStyle.gridSpace
            bottomPadding: ShinkiStyle.gridSpace
            Layout.preferredHeight: Math.ceil(backMetrics.height) + topPadding + bottomPadding

            contentItem: QQC2.Label {
                text: backButton.text
                color: ShinkiStyle.red
                font.family: ShinkiStyle.narrowFamily
                font.bold: true
                font.pixelSize: ShinkiStyle.columnSize
                font.letterSpacing: ShinkiStyle.columnSize * ShinkiStyle.columnTracking
                font.underline: backButton.hovered || backButton.visualFocus
            }
        }

        RowLayout {
            Layout.fillWidth: true
            // One height for every title (a Japanese title's font is taller): see above.
            Layout.preferredHeight: Math.ceil(Math.max(ShinkiStyle.rowMinSpace, titleMetrics.height))

            // The mark of the app: the club's sign in a red square, on the title's row (the clickdummy).
            Rectangle {
                objectName: "headLogo"

                visible: head.logo !== ""
                Layout.preferredWidth: ShinkiStyle.logoSize
                Layout.preferredHeight: ShinkiStyle.logoSize
                Layout.rightMargin: ShinkiStyle.gridSpace * 2
                color: ShinkiStyle.red

                QQC2.Label {
                    anchors.centerIn: parent
                    text: head.logo
                    color: ShinkiStyle.textOnRed
                    font.family: ShinkiStyle.japaneseFamily
                    font.pixelSize: ShinkiStyle.logoKanjiSize
                }
            }
            spacing: ShinkiStyle.gridSpace * 2

            QQC2.Label {
                Layout.fillWidth: !kanjiLabel.visible
                text: head.title
                color: ShinkiStyle.ink
                font.family: ShinkiStyle.uiFamily
                font.bold: true
                font.pixelSize: ShinkiStyle.pageTitleSize
                elide: Text.ElideRight
                Accessible.role: Accessible.Heading
            }
            QQC2.Label {
                id: kanjiLabel

                Layout.fillWidth: true
                visible: head.kanji !== ""
                text: head.kanji
                color: ShinkiStyle.inkMuted
                font.family: ShinkiStyle.japaneseFamily
                font.pixelSize: ShinkiStyle.uiSize
                elide: Text.ElideRight
            }

            Repeater {
                model: head.shown

                ShinkiToolButton {
                    required property var modelData

                    objectName: "headAction"
                    icon.name: modelData.icon.name
                    text: modelData.text
                    display: modelData.displayHint & Kirigami.DisplayHint.IconOnly ? QQC2.AbstractButton.IconOnly
                                                                                   : QQC2.AbstractButton.TextBesideIcon
                    enabled: modelData.enabled
                    checkable: modelData.checkable
                    checked: modelData.checked
                    onClicked: modelData.trigger()
                }
            }
            ShinkiToolButton {
                objectName: "headMore"
                visible: head.hidden.length > 0
                icon.name: "overflow-menu"
                text: head.moreText
                display: QQC2.AbstractButton.IconOnly
                onClicked: more.popup(this, 0, height)

                QQC2.Menu {
                    id: more

                    Repeater {
                        model: head.hidden

                        QQC2.MenuItem {
                            required property var modelData

                            text: modelData.text
                            icon.name: modelData.icon.name
                            enabled: modelData.enabled
                            onTriggered: modelData.trigger()
                        }
                    }
                }
            }
        }
    }
}
