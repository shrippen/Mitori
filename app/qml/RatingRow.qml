import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import Kante
import Mitori

// "How well does it sit?": the last rating and the four levels (sheet and player). A tap rates
// the entry now; the same level again counts as practice too.
ColumnLayout {
    id: rating

    required property string entry
    readonly property int level: Practice.revision >= 0 ? Practice.level(entry) : 0
    readonly property date last: Practice.revision >= 0 ? Practice.lastRated(entry) : new Date(NaN)

    spacing: KanteStyle.unit(6)

    QQC2.Label {
        Layout.fillWidth: true
        text: rating.level > 0
              ? qsTr("How well does it sit? Last: %1, %2.").arg(Practice.levelNames[rating.level])
                    .arg(rating.last.toLocaleDateString(Qt.locale(), Locale.ShortFormat))
              : qsTr("How well does it sit? Not rated yet.")
        color: KanteStyle.mutedTextColor
        wrapMode: Text.Wrap
    }
    KanteLevelPicker {
        Layout.fillWidth: true
        value: rating.level
        names: Practice.levelNames
        onPicked: level => Practice.rate(rating.entry, level)
        Accessible.name: qsTr("Rating")
    }
}
