import QtQuick

// Card (bible section 5, Karte): a ground for a group of content, paperAlt with a 1 px frame in
// line, square. As a control's background: background: ShinkiCard {}. Same use as KanteCard.
Rectangle {
    color: ShinkiStyle.paperAlt
    border.width: ShinkiStyle.lineWidth
    border.color: ShinkiStyle.line
}
