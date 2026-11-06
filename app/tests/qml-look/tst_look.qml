import QtQuick
import QtTest
import org.kde.kirigami as Kirigami
import Mitori

// Style and language change while the app runs (Appearance): the window built anew, the texts
// retranslated, nothing lost. Run by tst_ui from Leinen (ctest look) and from Shinki (look_shinki),
// in a file of its own: tst_ui.qml holds page types of the start's style through its own uses.
TestCase {
    id: test

    name: "Look"
    when: windowShown

    property var window: null
    readonly property int startStyle: uiShinki ? Appearance.Style.Shinki : Appearance.Style.Leinen
    readonly property int otherStyle: uiShinki ? Appearance.Style.Kante : Appearance.Style.Shinki
    readonly property string startName: uiShinki ? "Shinki" : "Leinen"
    readonly property string otherName: uiShinki ? "Kante" : "Shinki"
    readonly property string pages: "qrc:/qt/qml/Mitori/qml/"
    // As a page in tst_perf.qml: generous, the CI's machine is small.
    readonly property int switchBudget: 5000
    readonly property int scrolled: 200

    function initTestCase() {
        Appearance.setLanguage(Appearance.Language.English)
        const component = Qt.createComponent("Mitori", "Main")
        verify(component.status === Component.Ready, component.errorString())
        window = component.createObject(null, { width: 420, height: 800 })
        window.show()
        tryVerify(() => window.pageStack.currentItem !== null)
        compare(Appearance.window, window)
    }

    function init() {
        failOnWarning(/Binding loop/)
        failOnWarning(/ReferenceError/)
        failOnWarning(/TypeError/)
        failOnWarning(/is not a type/)
    }

    function cleanup() {
        Appearance.setLanguage(Appearance.Language.English)
        toStyle(startStyle)
        while (window.pageStack.depth > 1)
            window.pageStack.pop()
    }

    function allChildren(item) {
        let all = []
        for (let i = 0; i < item.children.length; i++) {
            all.push(item.children[i])
            all = all.concat(allChildren(item.children[i]))
        }
        return all
    }

    function findChildren(item, name) {
        return allChildren(item).filter(c => c.objectName === name)
    }

    // Applies a style as a tap does; returns when the new window stands.
    function toStyle(style) {
        if (Appearance.style === style && Appearance.running === style) {
            return
        }
        const old = Appearance.window
        Appearance.setStyle(style)
        tryVerify(() => Appearance.window !== old && Appearance.window !== null)
        window = Appearance.window
        tryVerify(() => window.pageStack.currentItem !== null)
    }

    function shot(name) {
        wait(300)
        let done = false
        window.pageStack.grabToImage(result => {
            result.saveToFile(uiShotsDir + "/" + name + ".png")
            done = true
        })
        tryVerify(() => done)
    }

    function push(page, properties) {
        window.pageStack.push(Qt.resolvedUrl(pages + page + ".qml"), properties || {})
        tryVerify(() => window.pageStack.currentItem && window.pageStack.currentItem.title !== undefined)
        return window.pageStack.currentItem
    }

    function openAppearance() {
        const settings = push("SettingsPage")
        findChild(settings, "appearanceButton").clicked()
        tryVerify(() => window.pageStack.depth === 3 && window.pageStack.currentItem.keptPage === "AppearancePage.qml")
        return window.pageStack.currentItem
    }

    function tile(page, name, text) {
        const found = findChildren(page, name).find(t => t.text === text)
        verify(found, name + " " + text)
        return found
    }

    function standingPreviews(page) {
        const all = findChildren(page, "lookPreview")
        verify(all.length > 0)
        tryVerify(() => all.every(p => p.ready), 20000, "every preview stands")
        return all
    }

    // The file selector reached the new window: Look, the head instead of Kirigami's bar, a
    // component's own file.
    function verifyStyle(shinki) {
        compare(Look.shinki, shinki)
        compare(window.pageStack.globalToolBar.style,
                shinki ? Kirigami.ApplicationHeaderStyle.None : Kirigami.ApplicationHeaderStyle.Auto)
        compare(findChild(window, "lookHead").implicitHeight > 0, shinki)
        compare(uiCatalog.drawnByShinki(findChild(window.pageStack.currentItem, "introButton")), shinki)
    }

    // Both ways: the same pages come back, the old window is gone, Back still works, the window
    // turns, the bars take the new ground.
    function test_style_both_ways() {
        const windows = uiCatalog.windowCount()
        const page = openAppearance()
        uiCatalog.watch(window)
        uiCatalog.watch(page)
        verifyStyle(uiShinki)
        const bar = String(Appearance.barColor)     // a copy: a colour read is a live reference
        verify(Qt.colorEqual(bar, Look.backgroundColor))
        standingPreviews(page)
        shot("look-" + startName)
        page.flickable.contentY = scrolled

        const started = Date.now()
        toStyle(otherStyle)
        const took = Date.now() - started
        console.log("change of style:", took, "ms")
        verify(took <= switchBudget, took + " ms")
        compare(uiCatalog.watchedAlive(), 0, "old window and page gone")
        compare(uiCatalog.windowCount(), windows)
        compare(Appearance.running, otherStyle)
        compare(window.pageStack.depth, 3)
        compare(window.pageStack.currentItem.keptPage, "AppearancePage.qml")
        compare(window.pageStack.get(1).keptPage, "SettingsPage.qml")
        compare(window.width, 420)
        compare(window.height, 800)
        verifyStyle(!uiShinki)
        tryVerify(() => window.pageStack.currentItem.flickable.contentY === scrolled, 5000, "the same place on the page")
        verify(Qt.colorEqual(Appearance.barColor, Look.backgroundColor))
        verify(!Qt.colorEqual(Appearance.barColor, bar), "the bars follow")
        standingPreviews(window.pageStack.currentItem)
        shot("look-" + otherName)

        // Sideways and back.
        window.width = 800
        window.height = 420
        wait(100)
        window.width = 420
        window.height = 800

        toStyle(startStyle)
        compare(window.pageStack.depth, 3)
        verifyStyle(uiShinki)
        verify(Qt.colorEqual(Appearance.barColor, bar))

        // Back through the kept pages, as the Back key.
        window.back()
        tryVerify(() => window.pageStack.currentItem.keptPage === "SettingsPage.qml")
        window.back()
        tryVerify(() => window.pageStack.currentIndex === 0)
    }

    // A tap on the tile itself, there and back; a tile of the running style does nothing.
    function test_style_by_tap() {
        const page = openAppearance()
        const old = window
        tile(page, "styleTile", startName).clicked()
        wait(50)
        compare(Appearance.window, old, "the running style: no new window")
        for (const name of [otherName, startName]) {
            const before = window
            tile(window.pageStack.currentItem, "styleTile", name).clicked()
            tryVerify(() => Appearance.window !== before)
            window = Appearance.window
            tryVerify(() => window.pageStack.depth === 3)
            standingPreviews(window.pageStack.currentItem)
        }
        compare(Appearance.running, startStyle)
        compare(Appearance.style, startStyle)
    }

    // The wizard: a change of style comes back on the same step.
    function test_style_in_wizard() {
        const welcome = push("WelcomePage")
        findChild(welcome, "welcomeNext").clicked()
        toStyle(otherStyle)
        compare(window.pageStack.depth, 2)
        compare(window.pageStack.currentItem.step, 1)
        compare(findChild(window.pageStack.currentItem, "welcomeSteps").currentIndex, 1)
        verify(findChildren(window.pageStack.currentItem, "styleTile").length >= 3)
    }

    // A note being typed when the window goes is saved (NoteRow); the service is the same one.
    function test_draft_kept() {
        const id = "draft-entry"
        const count = Practice.notes(id).length
        const service = Practice
        const page = window.pageStack.push(Qt.resolvedUrl("DraftPage.qml"), { entryId: id })
        findChild(page, "noteField").text = "Entwurf beim Stilwechsel"

        toStyle(otherStyle)
        const notes = Practice.notes(id)
        compare(notes.length, count + 1)
        verify(notes.some(n => n.text === "Entwurf beim Stilwechsel"))
        verify(Practice === service, "the service stayed")
    }

    // A text typed into Settings, still in focus when the style changes, is saved.
    function test_settings_field_kept() {
        const settings = push("SettingsPage")
        const field = findChild(settings, "videoFolder")
        const before = Videos.folder
        const folder = uiDataDir + "/some-folder"
        window.requestActivate()
        tryVerify(() => window.active)
        field.forceActiveFocus()
        tryVerify(() => field.activeFocus)
        field.text = folder

        toStyle(otherStyle)
        compare(Videos.folder, folder)
        compare(window.pageStack.currentItem.keptPage, "SettingsPage.qml")
        compare(findChild(window.pageStack.currentItem, "videoFolder").text, folder)
        Videos.folder = before
    }

    // German and English both ways, with pages there before the change: their texts, models built
    // in C++ (Practice.levelNames, Appearance.styles, AllNotes), dates and numbers.
    function test_language_both_ways() {
        const page = openAppearance()
        const settings = window.pageStack.get(1)
        const start = window.pageStack.get(0)
        const first = window
        const todayEnglish = findChild(start, "todayTile").text

        // A note on a technique that left the programme: its row's title comes from the catalog.
        verify(Practice.addNote("gone-from-programme", "note"))
        const notes = allNotes.createObject(test)
        const goneTitle = () => notes.rows.find(r => r.entry === "gone-from-programme").title
        compare(goneTitle(), "Technique no longer in the programme")
        compare(page.title, "Appearance")
        compare(Practice.levelNames[4], "exam-ready")

        tile(page, "languageTile", "Deutsch").clicked()
        compare(Appearance.language, Appearance.Language.German)
        compare(Appearance.window, first, "the window stays")
        compare(page.title, "Darstellung")
        compare(settings.title, "Einstellungen")
        verify(findChild(settings, "appearanceButton").text.indexOf("Stil") === 0)
        verify(findChild(start, "todayTile").text !== todayEnglish)
        compare(Practice.levelNames[4], "prüfungsreif")
        compare(goneTitle(), "Technik nicht mehr im Programm")
        tryVerify(() => findChildren(page, "styleTile").some(t => t.hint === "Kante in hellem Leinen"))
        compare(Qt.locale().name.substring(0, 2), "de")
        compare(Number(1.5).toLocaleString(Qt.locale(), "f", 1), "1,5")
        compare(new Date(2026, 0, 2).toLocaleDateString(Qt.locale(), "MMMM"), "Januar")
        standingPreviews(page)
        shot("look-de-" + startName)

        tile(page, "languageTile", "English").clicked()
        compare(page.title, "Appearance")
        compare(settings.title, "Settings")
        compare(findChild(start, "todayTile").text, todayEnglish)
        compare(Practice.levelNames[4], "exam-ready")
        compare(goneTitle(), "Technique no longer in the programme")
        compare(Qt.locale().name.substring(0, 2), "en")
        compare(Number(1.5).toLocaleString(Qt.locale(), "f", 1), "1.5")
        compare(new Date(2026, 0, 2).toLocaleDateString(Qt.locale(), "MMMM"), "January")
        notes.destroy()
    }

    // The language survives a change of style, and the new window's pages speak it.
    function test_language_then_style() {
        openAppearance()
        Appearance.setLanguage(Appearance.Language.German)
        toStyle(otherStyle)
        compare(window.pageStack.currentItem.title, "Darstellung")
        compare(window.pageStack.get(1).title, "Einstellungen")
        Appearance.setLanguage(Appearance.Language.English)
        compare(window.pageStack.currentItem.title, "Appearance")
    }

    // The previews: each style and language has one, drawn by the style's own components, its
    // texts in the language asked for; a tap on one applies it.
    function test_previews() {
        const page = openAppearance()
        const previews = standingPreviews(page)
        const styles = findChildren(page, "styleTile")
        const languages = findChildren(page, "languageTile")
        compare(languages.length, 3)
        compare(previews.length, styles.length + languages.length)

        const shinki = previews.find(p => p.look === Appearance.Style.Shinki)
        const leinen = previews.find(p => p.look === Appearance.Style.Leinen)
        verify(uiCatalog.drawnByShinki(findChild(shinki.sample, "sampleButton")), "Shinki's button")
        verify(!uiCatalog.drawnByShinki(findChild(leinen.sample, "sampleButton")), "Kante's button")

        // The language's tiles come after the styles': their words in the running style.
        const german = previews[styles.length + Appearance.Language.German]
        const english = previews[styles.length + Appearance.Language.English]
        compare(findChild(german.sample, "sampleChip").text, "prüfungsreif")
        compare(findChild(english.sample, "sampleChip").text, "exam-ready")
        compare(uiCatalog.drawnByShinki(findChild(german.sample, "sampleButton")), uiShinki)
        shot("look-previews-" + startName)

        // A tap on the sample chooses too.
        const flick = page.flickable
        flick.contentY = german.mapToItem(flick.contentItem, 0, 0).y - 20
        wait(50)
        const before = window
        mouseClick(german)
        compare(Appearance.language, Appearance.Language.German)
        compare(Appearance.window, before)
    }

    Component {
        id: allNotes

        AllNotes {
            practice: Practice
            courses: Courses
            programme: Programme
        }
    }
}
