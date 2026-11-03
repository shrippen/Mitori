.pragma library

// A comment text of shinkiryu.de as blocks for the reading column: paragraphs apart by an
// empty line, "## " starts a heading (the Kumidachi text: "## Rei-ho 礼法 … zu Beginn").
//   blocks("Intro\n\n## Rei-ho\n\nAlle sitzen …")
//   → [{heading: false, text: "Intro"}, {heading: true, text: "Rei-ho"}, {heading: false, …}]
const headingMark = "## "

function blocks(text) {
    const result = []
    for (const part of (text || "").split(/\n\s*\n/)) {
        const block = part.trim()
        if (block === "") {
            continue
        }
        if (block.startsWith(headingMark)) {
            result.push({ heading: true, text: block.slice(headingMark.length).trim() })
            continue
        }
        result.push({ heading: false, text: block })
    }
    return result
}
