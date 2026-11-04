import QtQuick
import QtQuick.Controls as QQC2

// The small caps line above a title or a field: narrow, spaced out, in the accent (the
// clickdummy's `.k`). The face and the spacing come from the style: PT Sans Narrow with 0.1 em
// under Shinki, the style's own face elsewhere.
QQC2.Label {
    color: Look.accentColor
    font: {
        const f = Look.narrowFont
        f.letterSpacing = (f.pixelSize > 0 ? f.pixelSize : f.pointSize) * Look.columnTracking
        f.capitalization = Font.AllUppercase
        return f
    }
    elide: Text.ElideRight
}
