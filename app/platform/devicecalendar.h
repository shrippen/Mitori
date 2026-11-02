#pragma once

#include <QDate>
#include <QString>

// The device's calendar app, to take over a course: Android opens its "new event" screen
// (Intent ACTION_INSERT on CalendarContract.Events) filled in; elsewhere an .ics file in the
// cache opens in the desktop's calendar (Merkuro, KOrganizer, Thunderbird …). The user saves it there.
namespace DeviceCalendar {

struct Event {
    QString uid;
    QString title;
    QDate first;
    QDate last;             // inclusive; a whole-day event
    QString place;
    QString description;
};

// False when no calendar app could be started.
bool add(const Event &event);

// The iCalendar text of one whole-day event (the desktop's way; tests).
QByteArray ics(const Event &event);

}
