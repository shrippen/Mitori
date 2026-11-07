import QtQuick

// A page's `wide` that the on-screen keyboard does not flip: the keyboard shrinks the window
// (adjustResize), which would change the columns under the fingers, both ways. From the moment a
// field in the window is typed in (also on a dialog or a page above) the page keeps its columns,
// until no field has the focus and the page is as high as when it began. A turn (new width and new
// height at once) counts at once, with the old width as the height: the keyboard may still be open,
// and the tablet is what counts, not the low window. A new width alone is no turn; held columns
// never stand in a window higher than wide.
QtObject {
    id: hold

    required property Item target

    readonly property bool wide: holding ? heldWide && target.width > target.height : Tablet.wide(target.width, target.height)

    property bool holding: false
    property bool heldWide: false
    property real heldHeight: 0

    readonly property real targetWidth: target.width
    readonly property bool high: target.height >= heldHeight
    // Text inputs have a cursor, whatever control they sit in (a field, a spin box, a date field).
    readonly property bool typing: {
        const item = target.Window.activeFocusItem
        return !!item && item.cursorPosition !== undefined
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
    property real lastHeight: 0
    readonly property real targetHeight: target.height
    Component.onCompleted: {
        lastWidth = targetWidth
        lastHeight = targetHeight
    }
    // A size change signals the width first: here the height is new, lastHeight not yet.
    onTargetWidthChanged: {
        if (target.height !== lastHeight) {
            holding = false
            if (typing) {
                begin(lastWidth)
            }
        }
        lastWidth = targetWidth
    }
    onTargetHeightChanged: lastHeight = targetHeight
}
