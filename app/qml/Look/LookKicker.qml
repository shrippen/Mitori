import QtQuick
import QtQuick.Controls as QQC2

// The small caps line above a title or a field: narrow, spaced out, in the accent (the
// clickdummy's `.k`). The face and the spacing come from the style: PT Sans Narrow with 0.1 em
// under Shinki, the style's own face elsewhere.
QQC2.Label {
    color: Look.accentColor
    // Qt.font: writing to a copy of Look.narrowFont changes nothing (no capitals, no spacing).
    font: {
        const f = Look.narrowFont
        const size = f.pixelSize > 0 ? { pixelSize: f.pixelSize } : { pointSize: f.pointSize }
        return Qt.font(Object.assign(size, {
            family: f.family,
            weight: f.weight,
            letterSpacing: (f.pixelSize > 0 ? f.pixelSize : f.pointSize) * Look.columnTracking,
            capitalization: Font.AllUppercase
        }))
    }
    elide: Text.ElideRight
}
