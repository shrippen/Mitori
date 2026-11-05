import QtQuick
import org.kde.kirigami as Kirigami

// Gives a page (or a dialog's content) Shinki's colours through Kirigami.Theme, so the controls
// it holds (still the platform's controls style) draw on paper in ink, red as the highlight.
Item {
    id: scope

    visible: false

    property Item target: null

    function apply() {
        if (!scope.target) {
            return
        }
        const theme = scope.target.Kirigami.Theme
        theme.inherit = false
        theme.backgroundColor = ShinkiStyle.paper
        theme.alternateBackgroundColor = ShinkiStyle.paperAlt
        theme.textColor = ShinkiStyle.ink
        theme.disabledTextColor = ShinkiStyle.inkMuted
        theme.highlightColor = ShinkiStyle.red
        theme.highlightedTextColor = ShinkiStyle.textOnRed
        theme.focusColor = ShinkiStyle.ink
        theme.linkColor = ShinkiStyle.red
    }

    onTargetChanged: apply()
    Component.onCompleted: apply()
}
