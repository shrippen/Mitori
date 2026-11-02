#include "icsreader.h"

#include <QRegularExpression>
#include <QStringList>
#include <QUrl>

namespace {

// A content line: its name (upper case), its parameters' text and its value.
struct Line {
    QString name;
    QString params;
    QString value;
};

// "NAME;PARAM=\"a:b\";X=1:value": the first ':' outside quotes ends name and parameters.
Line split(const QString &line)
{
    bool quoted = false;
    for (qsizetype i = 0; i < line.size(); ++i) {
        const QChar c = line.at(i);
        if (c == QLatin1Char('"')) {
            quoted = !quoted;
        } else if (c == QLatin1Char(':') && !quoted) {
            const QString head = line.left(i);
            const qsizetype semi = head.indexOf(QLatin1Char(';'));
            return {(semi < 0 ? head : head.left(semi)).toUpper(), semi < 0 ? QString() : head.mid(semi + 1),
                    line.mid(i + 1)};
        }
    }
    return {};
}

// TEXT values: "\n" (or "\N") a new line, "\," "\;" "\\" the character itself.
QString unescape(const QString &value)
{
    QString out;
    out.reserve(value.size());
    for (qsizetype i = 0; i < value.size(); ++i) {
        const QChar c = value.at(i);
        if (c != QLatin1Char('\\') || i + 1 == value.size()) {
            out += c;
            continue;
        }
        const QChar next = value.at(++i);
        out += (next == QLatin1Char('n') || next == QLatin1Char('N')) ? QChar(QLatin1Char('\n')) : next;
    }
    return out;
}

// 20260606 or 20260606T100000Z: the day.
QDate day(const QString &value)
{
    return QDate::fromString(value.left(8), QStringLiteral("yyyyMMdd"));
}

// The official announcement: a line "Ausschreibung: URL" (the URL to the line's end, as it is).
QString announcement(const QString &description)
{
    static const QRegularExpression line(QStringLiteral("^\\s*Ausschreibung:\\s*(https?://\\S+)\\s*$"),
                                         QRegularExpression::MultilineOption | QRegularExpression::CaseInsensitiveOption);
    const QRegularExpressionMatch m = line.match(description);
    if (!m.hasMatch()) {
        return {};
    }
    const QString url = m.captured(1);
    return QUrl(url, QUrl::StrictMode).isValid() ? url : QString();
}

} // namespace

QList<IcsReader::Event> IcsReader::read(const QByteArray &ics)
{
    // Unfold: a line break followed by a space or tab continues the line.
    QString text = QString::fromUtf8(ics);
    text.replace(QLatin1String("\r\n"), QLatin1String("\n"));
    text.replace(QRegularExpression(QStringLiteral("\n[ \t]")), QString());

    QList<Event> events;
    Event event;
    bool inEvent = false;
    int nested = 0;                 // inside VALARM (or another component) of the event
    bool dateEnd = false;           // DTEND was a date: the day after the last one
    for (const QString &raw : text.split(QLatin1Char('\n'))) {
        const Line l = split(raw);
        if (l.name == QLatin1String("BEGIN")) {
            if (l.value.compare(QLatin1String("VEVENT"), Qt::CaseInsensitive) == 0 && !inEvent) {
                inEvent = true;
                event = {};
                dateEnd = false;
            } else if (inEvent) {
                ++nested;
            }
            continue;
        }
        if (l.name == QLatin1String("END")) {
            if (inEvent && nested > 0) {
                --nested;
            } else if (inEvent) {
                inEvent = false;
                if (dateEnd && event.last.isValid()) {
                    event.last = event.last.addDays(-1);
                }
                if (!event.last.isValid() || event.last < event.first) {
                    event.last = event.first;
                }
                event.announcement = announcement(event.description);
                if (!event.uid.isEmpty() && event.first.isValid()) {
                    events << event;
                }
            }
            continue;
        }
        if (!inEvent || nested > 0) {
            continue;
        }

        if (l.name == QLatin1String("UID")) {
            event.uid = l.value.trimmed();
        } else if (l.name == QLatin1String("SUMMARY")) {
            event.title = unescape(l.value).trimmed();
        } else if (l.name == QLatin1String("LOCATION")) {
            event.place = unescape(l.value).trimmed();
        } else if (l.name == QLatin1String("DESCRIPTION")) {
            event.description = unescape(l.value).trimmed();
        } else if (l.name == QLatin1String("DTSTART")) {
            event.first = day(l.value);
        } else if (l.name == QLatin1String("DTEND")) {
            event.last = day(l.value);
            // A date (no time): the end is exclusive. A date-time ends on its own day.
            dateEnd = !l.value.contains(QLatin1Char('T'));
        }
    }
    return events;
}
