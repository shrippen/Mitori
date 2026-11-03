import Kante

// A group's head in a list: its title and the Japanese. Kante: a section heading with both in
// one line; Shinki (+shinki/LookGroupHead.qml): the red column, the Japanese in inkMuted.
KanteHeading {
    property string title: ""
    property string japanese: ""

    level: 4
    text: japanese !== "" ? title + "  " + japanese : title
}
