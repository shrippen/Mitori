import QtQuick
import QtTest
import org.kde.kirigami as Kirigami
import Mitori

// How long the pages take with five years of use (tst_perf.cpp). "PERF <name> <ms> cpu <ms> budget
// <ms>" for the record. Budgets hold CPU time: the time on the wall also counts who else runs (Player
// 5597 ms with 40 busy processes, 1312 ms CPU), and says nothing about the page.
TestCase {
    id: test

    name: "Perf"
    when: windowShown

    // Each case its own budget, ms CPU in a release build: 1.5 times the most measured alone (here
    // and on the GitHub runner), 1.25 times the most with 40 busy processes on 24 threads; a frame
    // (16) at least. A debug build has four times as much. A case without a budget fails.
    // Snappy is a tap answered within a frame and a page drawn within about 200 ms. Above that
    // today, to be made faster, not the budget looser: "slow".
    readonly property var budgets: ({
        "start menu": 950,                  // slow: cold start, first QML types
        "TodayPage": 850,                   // slow: heatmap
        "StatsPage": 800,                   // slow: heatmap
        "YearsPage": 1300,                  // slow: five years of heatmap, 1800 days
        "AllNotesPage": 400,                // slow
        "TechniqueMapPage": 250,
        "SearchPage": 200,
        "CoursesPage": 90,
        "NotebooksPage": 80,
        "GradesPage": 350,
        "SettingsPage": 70,
        "BackupPage": 160,
        "VideosPage": 120,
        "AppearancePage": 550,              // slow: previews
        "ExamPage": 180,
        "Programme aikido": 350,
        "Programme daitoryu": 400,
        "Programme shinkitoho": 350,
        "Programme tablet": 700,            // slow: list, page and small player
        "Player first": 1900,               // slow: Qt probes the GPU's decoders, 1.1 s with NVIDIA
        "Programme.entries": 16,
        "Practice.due": 16,
        "Practice.counts": 16,
        "Practice.allNotes": 110,
        "Practice.searchNotes": 20,
        "Practice.practisedPerDay": 20,
        "Practice.practisedInYear": 40,
        "Programme.suggest, 500 words": 110,
        "Search": 400,
        "Search “k”": 600,                  // slow: a keystroke
        "Search “ko”": 500,                 // slow
        "Search “kot”": 300,                // slow
        "Search “kote”": 130,
        "Programme": 300,
        "Player over programme": 650,
        "Practice.rate": 20,
        "Practice.addNote": 60,
        "Stats again": 1500,                // slow
        "Years over stats": 3100,           // slow: see YearsPage
        "Today over years": 850,            // slow
        "Player over stats": 850,
        "Practice.rate under stats": 1200   // slow: a tap; YearsPage below counts anew
    })
    readonly property int debugFactor: perfClock.debugBuild() ? 4 : 1
    property var window: null
    property var times: []

    function initTestCase() {
        const component = Qt.createComponent("Mitori", "Main")
        verify(component.status === Component.Ready, component.errorString())
        const started = now()
        window = component.createObject(null, { width: 420, height: 800 })
        window.show()
        tryVerify(() => window.pageStack.currentItem !== null, 10000)
        waitForRendering(window.contentItem)
        report("start menu", started)
    }

    function cleanupTestCase() {
        console.log("PERF slowest " + times.sort((a, b) => b.ms - a.ms).slice(0, 5).map(t => t.name + " " + t.ms).join(", "))
        window.destroy()
    }

    // Kirigami 6.31 crashes on a pop in the last moments of a slide: ColumnView::setCurrentIndex
    // hands the focus to the page below, a Behavior on that makes Qt run the slide's end there and
    // then, which takes the current item away (SIGSEGV in QQuickItem::size). A tap does not get
    // here, a page does not take taps while it slides; a test does.
    function popPage() {
        tryVerify(() => !window.pageStack.columnView.moving, 5000, "the slide ends")
        // A page may have gone during the wait (TrainingEndPage): the start menu stays.
        if (window.pageStack.depth > 1) {
            window.pageStack.pop()
        }
    }

    // A failed budget must not leave its page in the stack (it would slow what comes next).
    function cleanup() {
        while (window.pageStack.depth > 1) {
            popPage()
        }
        wait(50)
    }

    function now() {
        return { wall: Date.now(), cpu: perfClock.cpuMs() }
    }

    // Since `started`; `waited`: what the case itself waits for, not part of the wall's time.
    function report(name, started, waited) {
        const wall = Date.now() - started.wall - (waited || 0)
        const ms = perfClock.cpuMs() - started.cpu
        const budget = budgets[name] * debugFactor
        console.log("PERF " + name + " " + wall + " cpu " + ms + " budget " + budget)
        times.push({ name: name, ms: ms })
        verify(!isNaN(budget), "no budget for " + name)
        verify(ms <= budget, name + " took " + ms + " ms CPU, budget " + budget)
    }

    // A page from the push until it is drawn.
    function page(name, file, properties) {
        const started = now()
        window.pageStack.push(Qt.resolvedUrl("qrc:/qt/qml/Mitori/qml/" + file + ".qml"), properties || {})
        waitForRendering(window.contentItem)
        report(name, started)
        const shown = window.pageStack.currentItem
        wait(350)   // the slide, not part of the time
        return shown
    }

    function back() {
        popPage()
        wait(50)
    }

    function call(name, f) {
        const started = now()
        const result = f()
        report(name, started)
        return result
    }

    function test_1_pages() {
        for (const name of ["TodayPage", "StatsPage", "YearsPage", "AllNotesPage", "TechniqueMapPage", "SearchPage",
                            "CoursesPage", "NotebooksPage", "GradesPage", "SettingsPage", "BackupPage", "VideosPage", "AppearancePage", "ExamPage"]) {
            page(name, name)
            back()
        }
    }

    function test_2_programmes() {
        for (const discipline of ["aikido", "daitoryu", "shinkitoho"]) {
            Programme.discipline = discipline
            page("Programme " + discipline, "ProgrammePage")
            back()
        }
        Programme.discipline = "aikido"
    }

    // Beside the list on a tablet the page has the small player too.
    function test_3_tablet() {
        window.width = 1280
        window.height = 800
        wait(200)
        page("Programme tablet", "ProgrammePage")
        back()
        window.width = 420
        window.height = 800
        wait(200)
    }

    function test_4_player() {
        const rows = Programme.entries("aikido", "k5").filter(r => r.kind === "entry")
        page("Player first", "PlayerPage", { playlist: rows, index: 0 })
        back()
    }

    function test_5_calls() {
        const rows = Programme.entries("aikido", "k5").filter(r => r.kind === "entry")
        call("Programme.entries", () => Programme.entries("aikido", "k5"))
        call("Practice.due", () => Practice.due(rows))
        call("Practice.counts", () => Practice.counts(rows.map(r => r.id)))
        call("Practice.allNotes", () => Practice.allNotes())
        call("Practice.searchNotes", () => Practice.searchNotes("Hüfte"))
        call("Practice.practisedPerDay", () => Practice.practisedPerDay(84))
        call("Practice.practisedInYear", () => Practice.practisedInYear(new Date().getFullYear(), "aikido"))
    }

    // A long note in the technique picker: it asks on the UI thread.
    function test_5b_suggest_long_note() {
        const sentence = "Heute Ikkyo aus Katate-dori geübt, dann die Hüfte tief. Danach Kote-gaeshi, langsam und sauber! "
        const words = sentence.trim().split(" ").length
        const text = sentence.repeat(Math.ceil(500 / words))
        call("Programme.suggest, 500 words", () => Programme.suggest(text))
    }

    // One keystroke in the search: the list follows at once.
    function test_6_search_typing() {
        const search = page("Search", "SearchPage")
        const field = findChild(search, "searchField")
        for (const text of ["k", "ko", "kot", "kote"]) {
            const started = now()
            field.text = text
            wait(320)   // the page waits for the typing to pause
            wait(0)     // the list has followed (no waitForRendering: an unchanged picture never comes)
            report("Search “" + text + "”", started, 320)
        }
        back()
    }

    // The save that everyone does: a rating with the list and the player open (Practice.changed).
    function test_7_rate() {
        const rows = Programme.entries("aikido", "k5").filter(r => r.kind === "entry")
        page("Programme", "ProgrammePage")
        page("Player over programme", "PlayerPage", { playlist: rows, index: 0 })
        call("Practice.rate", () => Practice.rate(rows[0].id, 3))
        call("Practice.addNote", () => Practice.addNote(rows[0].id, "Noch einmal"))
    }

    // The same with the statistics: the pages that count everything, under the player.
    function test_8_rate_over_stats() {
        const rows = Programme.entries("aikido", "k5").filter(r => r.kind === "entry")
        page("Stats again", "StatsPage")
        page("Years over stats", "YearsPage")
        page("Today over years", "TodayPage")
        page("Player over stats", "PlayerPage", { playlist: rows, index: 0 })
        call("Practice.rate under stats", () => Practice.rate(rows[0].id, 2))
    }
}
