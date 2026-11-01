#pragma once

#include <QDate>
#include <QHash>
#include <QList>
#include <QString>

#include "practicelog.h"

// Today's practice ("Wiederholen nach Plan", clickdummy "Heute üben"): what is unsure or long
// unpractised comes back after a number of days per level, and up to two new techniques of the
// current grade join. A technique rated today counts as practised and is not due again today.
namespace Schedule {

struct Candidate {
    QString id;
    bool currentGrade = false;  // part of the grade chosen in the programme (new ones come from there)
};

struct Due {
    QString id;
    int level = 0;              // 0: new
    int days = 0;               // since the last rating (0 for new)
};

// Days until a technique of level 1..4 is due again: [seen, with help, sure, exam-ready].
QList<int> defaultIntervals();

// Due candidates in order: rated ones by urgency (lower level and longer ago first, at most
// `maxRated`), then up to `maxNew` new ones of the current grade in programme order.
QList<Due> due(const QList<Candidate> &candidates, const QHash<QString, PracticeLog::Rating> &latest,
               const QDate &today, const QList<int> &intervals, int maxRated = 8, int maxNew = 2);

} // namespace Schedule
