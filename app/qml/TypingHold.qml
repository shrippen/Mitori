import QtQuick

// A page's `wide` that the on-screen keyboard does not flip: the keyboard shrinks the window
// (adjustResize), which would change the columns under the fingers, both ways. From the moment a
// field of `target` is typed in the page keeps its columns, until no field has the focus and the
// page is as high as when it began. A new width (turned) counts at once, with the old width as the
// height: the keyboard may still be open, and the tablet is what counts, not the low window.
QtObject {
    id: hold

    required property Item target

    readonly property bool wide: holding ? heldWide : Tablet.wide(target.width, target.height)

    property bool holding: false
    property bool heldWide: false
    property real heldHeight: 0

    readonly property real targetWidth: target.width
    readonly property bool high: target.height >= heldHeight
    // Text inputs have a cursor, whatever control they sit in (a field, a spin box, a date field).
    readonly property bool typing: {
        const item = target.Window.activeFocusItem
        if (!item || item.cursorPosition === undefined) {
            return false
        }
        for (let p = item; p; p = p.parent) {
            if (p === target) {
                return true
            }
        }
        return false
    }

    function begin(fullHeight) {
        heldWide = Tablet.wide(target.width, fullHeight)
        heldHeight = target.height
        holding = true
    }
    function end() {
        if (holding && !typing && high) {
            holding = false
        }
    }

    onTypingChanged: {
        if (typing && !holding) {
            begin(target.height)
        } else {
            end()
        }
    }
    onHighChanged: end()
    property real lastWidth: 0
    Component.onCompleted: lastWidth = targetWidth
    onTargetWidthChanged: {
        holding = false
        if (typing) {
            begin(lastWidth)
        }
        lastWidth = targetWidth
    }
}
