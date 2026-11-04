import QtQuick
import QtQuick.Controls as QQC2

// A text in the look's colours: the ink of the style and its face. A page names another colour
// (Look.mutedTextColor) or another face where it wants one (Look.readingFont for running text).
// Under Shinki the roles come from ShinkiStyle (paper, ink, PT Sans), under the other styles
// from KanteStyle: the text belongs to the style, not to the page.
QQC2.Label {
    color: Look.textColor
    font: Look.defaultFont
}
