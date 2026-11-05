pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import Mitori

// Glossary and search (clickdummy "Glossar & Suche"). Without a query: the glossary A–Z (word,
// kanji, German translation, a speaker button). With a query: techniques of every discipline and
// grade by name, attack (with its addition), group or Japanese, the glossary (also by German
// words) and the own notes; the glossary first, as the techniques can be up to 50 rows. Case, accents, hyphens and okurigana do not matter; several words
// must all match. A technique opens its sheet, a note its notes page.
//
//   ListView (virtual: the glossary has about 300 words)
//   ├─ header   hints
//   ├─ rows     the glossary, under A … Z headings without a query
//   └─ footer   techniques and notes found, source of the translations
Kirigami.ScrollablePage {
    id: page

    // ScrollablePage keeps the platform's colours (Theme.inherit: false): give it the style's, and
    // the style's page ground (Look: Kante or Shinki).
    property Item lookScope: LookScope { target: page }
    background: Rectangle { color: Look.backgroundColor }

    // The query searched (follows the field after a short pause while typing).
    property string query: ""
    readonly property bool searching: query.trim() !== ""
    // At most `limit` techniques are listed; one more found says there are more.
    readonly property int limit: 50
    readonly property var techniques: !searching ? [] : Programme.search(query, limit + 1)
    readonly property bool more: techniques.length > limit
    readonly property var shown: more ? techniques.slice(0, limit) : techniques
    readonly property var notes: !searching || Practice.notesRevision < 0 ? [] : Practice.searchNotes(query, limit + 1)
    readonly property bool notesMore: notes.length > limit
    readonly property var notesShown: notesMore ? notes.slice(0, limit) : notes
    // The glossary rows: every word, or the words found.
    readonly property var glossary: searching ? Glossary.search(query) : Glossary.words
    readonly property bool canSpeak: Glossary.speech === Glossary.Ready

    title: qsTr("Glossary & search")

    // A new query shows its results from the top (the list keeps its position when the header
    // with the techniques found changes height, which left it above the first row).
    onQueryChanged: Qt.callLater(list.positionViewAtBeginning)

    header: LookToolBar {
        LookTextField {
            id: field

            objectName: "searchField"
            anchors.fill: parent
            placeholderText: qsTr("Kiriotoshi, 切り落とし or Hüftwurf")
            Accessible.name: qsTr("Search")
            Component.onCompleted: forceActiveFocus()
            onTextChanged: pause.restart()
            onAccepted: page.query = text
            // Down goes into the glossary.
            Keys.onDownPressed: {
                list.currentIndex = Math.max(0, list.currentIndex)
                list.forceActiveFocus()
            }
        }
    }

    Timer {
        id: pause

        interval: 200
        onTriggered: page.query = field.text
    }

    ListView {
        id: list

        objectName: "glossaryList"
        model: page.glossary
        reuseItems: true
        currentIndex: -1
        activeFocusOnTab: true
        // A–Z headings in the whole glossary; words found stand under "Glossary".
        section.property: page.searching ? "" : "letter"
        section.delegate: LookSectionLabel {
            required property string section

            objectName: "letterHeading"
            width: ListView.view.width
            topPadding: Kirigami.Units.largeSpacing
            bottomPadding: Kirigami.Units.smallSpacing
            leftPadding: Kirigami.Units.largeSpacing
            text: section
        }

        // Up from the first row goes back to the field; Return or Space says the current word.
        Keys.onUpPressed: event => {
            if (currentIndex > 0) {
                event.accepted = false
                return
            }
            field.forceActiveFocus()
        }
        Keys.onReturnPressed: page.sayCurrent()
        Keys.onEnterPressed: page.sayCurrent()
        Keys.onSpacePressed: page.sayCurrent()

        // Hints and headings keep the page's margin; rows bring their own.
        header: ColumnLayout {
            width: ListView.view.width
            spacing: Kirigami.Units.largeSpacing
            onHeightChanged: Qt.callLater(list.positionViewAtBeginning)

            LookLabel {
                objectName: "glossaryIntro"
                Layout.fillWidth: true
                Layout.topMargin: Kirigami.Units.largeSpacing
                Layout.leftMargin: Kirigami.Units.largeSpacing
                Layout.rightMargin: Kirigami.Units.largeSpacing
                visible: !page.searching
                text: qsTr("%n Japanese term(s), with kanji and German translation where the lexicon or the programme has them. The search also finds German words, techniques and your notes.", "", Glossary.words.length)
                color: Look.mutedTextColor
                wrapMode: Text.Wrap
            }

            LookLabel {
                objectName: "speechHint"
                Layout.fillWidth: true
                Layout.leftMargin: Kirigami.Units.largeSpacing
                Layout.rightMargin: Kirigami.Units.largeSpacing
                visible: Glossary.speech === Glossary.NoVoice || Glossary.speech === Glossary.Unsupported
                text: Glossary.speech === Glossary.Unsupported
                      ? qsTr("This version of Mitori has no speech output, so there is no pronunciation.")
                      : qsTr("No Japanese voice for speech output found. Where the system offers one (Android: settings, text-to-speech output), install it to hear the pronunciation.")
                color: Look.mutedTextColor
                wrapMode: Text.Wrap
            }

            LookLabel {
                Layout.fillWidth: true
                Layout.leftMargin: Kirigami.Units.largeSpacing
                Layout.rightMargin: Kirigami.Units.largeSpacing
                visible: page.searching && page.techniques.length === 0 && page.notes.length === 0 && page.glossary.length === 0
                text: qsTr("Nothing found for “%1”.").arg(page.query.trim())
                color: Look.mutedTextColor
                wrapMode: Text.Wrap
            }

            LookSectionLabel {
                Layout.leftMargin: Kirigami.Units.largeSpacing
                visible: page.searching && page.glossary.length > 0
                text: qsTr("Glossary")
            }
        }

        delegate: LookListRow {
            id: row

            required property var modelData
            required property int index

            objectName: "glossaryRow"
            width: ListView.view.width
            // The list is one Tab stop; Up and Down move in it.
            activeFocusOnTab: false
            focusOnClick: false
            selected: ListView.isCurrentItem && list.activeFocus
            text: modelData.word + (modelData.jp ? "  " + modelData.jp : "")
            subtitle: modelData.de || qsTr("No translation in the lexicon")
            // A tap, or Return and Space on the current row (the row takes them as a click), says it.
            onClicked: {
                list.currentIndex = index
                Glossary.say(modelData.speech)
            }
            trailing: LookToolButton {
                objectName: "sayButton"
                visible: page.canSpeak
                focusPolicy: Qt.NoFocus
                display: QQC2.AbstractButton.IconOnly
                icon.name: "media-playback-start"
                text: qsTr("Pronunciation")
                Accessible.name: qsTr("Pronounce %1").arg(row.modelData.word)
                onClicked: Glossary.say(row.modelData.speech)
            }
        }

        footer: ColumnLayout {
            width: ListView.view.width
            spacing: Kirigami.Units.largeSpacing

            LookSectionLabel {
                Layout.topMargin: Kirigami.Units.largeSpacing
                Layout.leftMargin: Kirigami.Units.largeSpacing
                visible: page.techniques.length > 0
                text: qsTr("Techniques")
            }
            Repeater {
                model: page.shown

                LookListRow {
                    required property var modelData

                    objectName: "techniqueResult"
                    Layout.fillWidth: true
                    leadingText: modelData.nr ? String(modelData.nr) : ""
                    // As in the programme: the technique's and the attack's addition tell rows apart.
                    text: modelData.title + (modelData.extra && !modelData.nr ? " (" + modelData.extra + ")" : "")
                          + (modelData.jp ? "  " + modelData.jp : "")
                    subtitle: [modelData.disciplineName + " " + modelData.gradeName, modelData.groupName,
                               modelData.attack + (modelData.attackExtra ? " (" + modelData.attackExtra + ")" : ""),
                               modelData.nr ? modelData.extra : ""].filter(x => x).join(" · ")
                    onClicked: {
                        sheet.entry = modelData
                        sheet.playable = !!modelData.video && Videos.available(modelData.video.video)
                        sheet.open()
                    }
                }
            }

            LookLabel {
                objectName: "moreHint"
                Layout.fillWidth: true
                Layout.leftMargin: Kirigami.Units.largeSpacing
                Layout.rightMargin: Kirigami.Units.largeSpacing
                visible: page.more
                text: qsTr("More than %1 techniques found. Add a word to narrow the search.").arg(page.limit)
                color: Look.mutedTextColor
                wrapMode: Text.Wrap
            }

            LookSectionLabel {
                Layout.topMargin: Kirigami.Units.largeSpacing
                Layout.leftMargin: Kirigami.Units.largeSpacing
                visible: page.notes.length > 0
                text: qsTr("Notes")
            }
            Repeater {
                model: page.notesShown

                LookListRow {
                    required property var modelData
                    // A technique no longer in the programme (a newer catalogue) keeps its notes.
                    readonly property var found: Programme.entry(modelData.entry)
                    readonly property var entry: found.id ? found : { id: modelData.entry, title: qsTr("Technique no longer in the programme") }

                    objectName: "noteResult"
                    Layout.fillWidth: true
                    text: modelData.text.replace(/\s+/g, " ")       // one line, as the row shows it
                    subtitle: [entry.title, modelData.created.toLocaleDateString(Qt.locale(), Locale.ShortFormat)].filter(x => x).join(" · ")
                    onClicked: page.Kirigami.PageStack.push(Qt.resolvedUrl("NotesPage.qml"), { entry: entry })
                }
            }

            LookLabel {
                objectName: "notesMoreHint"
                Layout.fillWidth: true
                Layout.leftMargin: Kirigami.Units.largeSpacing
                Layout.rightMargin: Kirigami.Units.largeSpacing
                visible: page.notesMore
                text: qsTr("More than %1 notes found. Add a word to narrow the search.").arg(page.limit)
                color: Look.mutedTextColor
                wrapMode: Text.Wrap
            }

            LookLabel {
                objectName: "glossarySource"
                Layout.fillWidth: true
                Layout.margins: Kirigami.Units.largeSpacing
                visible: page.glossary.length > 0
                text: qsTr("Translations: %1").arg(Glossary.source)
                font: Look.smallFont
                color: Look.mutedTextColor
                wrapMode: Text.Wrap
            }
        }
    }

    function sayCurrent() {
        if (list.currentIndex >= 0 && list.currentIndex < page.glossary.length) {
            Glossary.say(page.glossary[list.currentIndex].speech)
        }
    }

    TechniqueSheet {
        id: sheet

        objectName: "techniqueSheet"
        onPlayRequested: entry => {
            sheet.close()
            const window = page.QQC2.ApplicationWindow.window as Kirigami.ApplicationWindow
            window.pageStack.push(Qt.resolvedUrl("PlayerPage.qml"), { playlist: [entry], index: 0 })
        }
        onCommentRequested: entry => page.Kirigami.PageStack.push(Qt.resolvedUrl("CommentPage.qml"), { entry: entry })
        onNotesRequested: entry => page.Kirigami.PageStack.push(Qt.resolvedUrl("NotesPage.qml"), { entry: entry })
        onVideosRequested: page.Kirigami.PageStack.push(Qt.resolvedUrl("VideosPage.qml"))
    }
}
