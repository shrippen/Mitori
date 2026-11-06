pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import Mitori

// The end of a training (clickdummy "session", "Geschafft"): how many were practised, that the
// day counts. It takes the player's place on the stack; every way back leads to the page below.
Kirigami.ScrollablePage {
    id: page

    // ScrollablePage keeps the platform's colours (Theme.inherit: false): give it the style's, and
    // the style's page ground (Look: Kante or Shinki).
    property Item lookScope: LookScope { target: page }
    background: Rectangle { color: Look.backgroundColor }

    required property int count

    title: qsTr("Done")

    function close() {
        page.Kirigami.PageStack.pageStack.currentIndex = page.Kirigami.ColumnView.index - 1
    }

    // Keys reach Esc from the start. The Back key only moves the stack on: the page goes as well,
    // so that no swipe leads back here.
    onIsCurrentPageChanged: {
        if (isCurrentPage) {
            forceActiveFocus()
            return
        }

        const stack = page.Kirigami.PageStack.pageStack
        const place = page.Kirigami.ColumnView.index
        if (place < 0 || stack.currentIndex >= place) {
            return
        }
        Qt.callLater(() => stack.pop(stack.currentItem))
    }
    Keys.onEscapePressed: close()

    ColumnLayout {
        spacing: Kirigami.Units.largeSpacing

        LookKpi {
            objectName: "practisedCount"
            value: String(page.count)
            label: qsTr("technique(s) practised and rated", "", page.count)
        }
        LookLabel {
            Layout.fillWidth: true
            text: qsTr("Today counts as a practice day in the statistics.")
            color: Look.mutedTextColor
            wrapMode: Text.Wrap
        }
        LookButton {
            objectName: "backToToday"
            role: LookButton.Role.Primary
            text: qsTr("Back to today")
            onClicked: page.close()
        }
    }
}
