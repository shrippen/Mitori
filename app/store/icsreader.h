#pragma once

#include <QByteArray>
#include <QDate>
#include <QList>
#include <QString>

// The events of an iCalendar file (RFC 5545) as the course calendar needs them: the public
// calendar of the association's courses (ROADMAP.md, "Lehrgänge").
//
//   BEGIN:VEVENT                         lines folded (CRLF + space) are joined first;
//   UID:df0bce34-…                       \n \, \; \\ in values are resolved;
//   SUMMARY:Daitoryu Aiki Jujutsu        a parameter value in quotes may hold ':' (ALTREP);
//   DTSTART;VALUE=DATE:20260606          dates or date-times (the day counts);
//   DTEND;VALUE=DATE:20260608            DTEND of a date is the day after the last one;
//   DESCRIPTION;ALTREP="data:…":mit …    the text, not the HTML of ALTREP;
//   BEGIN:VALARM … END:VALARM            a reminder's own DESCRIPTION is not the event's.
//   END:VEVENT
class IcsReader
{
public:
    struct Event {
        QString uid;
        QString title;
        QDate first;            // the first day
        QDate last;             // the last day (inclusive)
        QString place;
        QString description;
        // A line "Ausschreibung: https://…" in the description, the URL as written (it holds & and
        // %2F, which must reach the server unchanged), else empty.
        QString announcement;
    };

    // Every event with a uid and a first day, in the order of the file.
    static QList<Event> read(const QByteArray &ics);
};
