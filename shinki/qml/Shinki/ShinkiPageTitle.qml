pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls as QQC2

// Page title (bible section 5, Kopf): the title of a Kirigami page in PT Sans bold 22 px, ink.
// Place it inside the page: ShinkiPageTitle { page: page }. Same interface as KantePageTitle.
Item {
    id: skin

    required property var page

    visible: false

    readonly property Component title: Component {
        QQC2.Label {
            text: skin.page ? skin.page.title : ""
            color: ShinkiStyle.ink
            font.family: ShinkiStyle.uiFamily
            font.bold: true
            font.pixelSize: ShinkiStyle.pageTitleSize
            elide: Text.ElideRight
            verticalAlignment: Text.AlignVCenter
        }
    }

    Binding {
        target: skin.page
        property: "titleDelegate"
        value: skin.title
        when: skin.page !== null
        restoreMode: Binding.RestoreBindingOrValue
    }
}
