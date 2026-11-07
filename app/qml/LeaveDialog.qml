import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import Mitori

// Before typed text goes (owner, 07.11.2026: a text about to be lost asks first). With `stackPage` it
// asks on Back (key, gesture, the head's arrow) while `asking`; Main asks it before the window
// closes; a dialog (NoteEditor) asks itself. Cancel stays, as if nothing was pressed.
//
//   ┌ Save the note? ──────────────┐     without `save` (a form not made yet):
//   │ “Hüfte früher …” is not saved │     ┌ Discard the new course? ─┐
//   │                   [Discard]   │     │ …                        │
//   │         [Cancel]  [Save]      │     │     [Cancel]  [Discard]  │
//   └───────────────────────────────┘     └──────────────────────────┘
//
// The third choice stands above the buttons, as in BackupPage: three do not fit one row on a phone.
LookDialog {
    id: dialog

    // The page that asks on Back; null for a dialog's own question.
    property Item stackPage: null
    // There is something to lose now.
    property bool asking: false
    // Keeps the text: true when it is kept (else the page shows why and stays). null: no saving.
    property var save: null
    // Drops the text.
    property var discard: () => {}
    property string text: ""
    // Cancel: nothing goes (NoteEditor opens again).
    signal stayed()

    readonly property bool savable: save !== null
    // What follows a choice that leaves (Save, Discard); null: nothing.
    property var then: null

    function ask(next) {
        then = next
        open()
    }
    // Leaving with a question first, where it has to ask ("All notes" back to the list).
    function leaveOrAsk() {
        if (asking) {
            ask(leave)
            return
        }
        leave()
    }
    // Out of the stack, with any page ahead of it (Back keeps those).
    function leave() {
        const stack = dialog.stackOf()
        const place = dialog.stackPage.Kirigami.ColumnView.index
        if (place > 0) {
            stack.pop(stack.get(place - 1))
        }
    }

    // Not saved (the page shows why): nothing follows, also not later.
    function keep() {
        if (!save()) {
            then = null
            return
        }
        go()
    }
    function drop() {
        discard()
        go()
    }
    function go() {
        const next = then
        then = null
        if (next) {
            next()
        }
    }

    parent: QQC2.Overlay.overlay
    anchors.centerIn: parent
    modal: true
    span: LookDialog.Span.Small
    destructive: !savable
    standardButtons: QQC2.Dialog.Ok | QQC2.Dialog.Cancel
    acceptText: savable ? qsTr("Save") : qsTr("Discard")
    contentHeight: body.implicitHeight
    onAccepted: savable ? keep() : drop()
    onRejected: {
        then = null
        stayed()
    }

    // A step back from the page, however it came, returns here and asks. A pop takes the page off
    // the stack: no question then.
    //   Back key, the head's arrow  goBack: backRequested, then the step (whatever `accepted`
    //                               says, as ExamEndPage). At once: Main.back pops the page next.
    //   a swipe, anything else      no backRequested; ColumnView steps back again as its slide
    //                               ends, and the finger lifts beside the dialog (closing it): the
    //                               question waits until the view stands.
    property bool backKey: false
    property bool settling: false
    function checkBack() {
        const page = dialog.stackPage
        const stack = page.Kirigami.PageStack.pageStack
        const place = page.Kirigami.ColumnView.index
        const byKey = dialog.backKey
        dialog.backKey = false
        if (!dialog.asking || page.isCurrentPage || place < 0 || stack.currentIndex >= place) {
            return
        }
        if (!byKey && (stack.columnView.dragging || stack.columnView.moving)) {
            dialog.settling = true
            return
        }
        Qt.callLater(() => {
            if (stack.get(place) !== page || !dialog.asking) {
                return
            }
            stack.currentIndex = place
            dialog.ask(dialog.leave)
        })
    }
    function stackOf() {
        return (dialog.stackPage.QQC2.ApplicationWindow.window as Kirigami.ApplicationWindow).pageStack
    }
    function settle() {
        const view = dialog.stackOf().columnView
        if (view.dragging || view.moving) {
            return
        }
        dialog.settling = false
        dialog.checkBack()
    }
    Connections {
        target: dialog.stackPage
        ignoreUnknownSignals: true

        function onBackRequested() {
            dialog.backKey = true
        }
        function onIsCurrentPageChanged() {
            dialog.checkBack()
        }
    }
    Connections {
        target: dialog.settling ? dialog.stackOf().columnView : null

        function onDraggingChanged() {
            dialog.settle()
        }
        function onMovingChanged() {
            Qt.callLater(dialog.settle)
        }
    }

    ColumnLayout {
        id: body

        width: parent.width
        spacing: Look.unit(8)

        LookLabel {
            objectName: "leaveText"
            Layout.fillWidth: true
            wrapMode: Text.Wrap
            // Empty while closed: a turning window wraps a closed dialog's text anew (BackupPage).
            text: dialog.visible ? dialog.text : ""
        }
        LookButton {
            objectName: "leaveDiscard"
            Layout.alignment: Qt.AlignRight
            visible: dialog.savable
            icon.name: "edit-delete"
            text: qsTr("Discard")
            onClicked: {
                dialog.close()
                dialog.drop()
            }
        }
    }
}
