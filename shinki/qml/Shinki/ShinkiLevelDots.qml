import QtQuick
import QtQuick.Controls as QQC2

// Level dots (bible section 5, `.li`): `count` round dots of 8 px, the first `value` filled in
// `color` (the level's), the others in `line`. Screen readers hear the level's name.
Row {
    id: dots

    property int value: 0
    property int count: 4
    property var names: []
    property color color: ShinkiStyle.red

    spacing: ShinkiStyle.gridSpace / 2
    Accessible.role: Accessible.Indicator
    Accessible.name: dots.names.length > dots.value ? String(dots.names[dots.value]) : String(dots.value)

    Repeater {
        model: dots.count

        Rectangle {
            required property int index

            width: ShinkiStyle.levelDotSize
            height: width
            radius: width / 2
            color: index < dots.value ? dots.color : ShinkiStyle.line
        }
    }
}
