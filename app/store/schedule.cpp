#include "schedule.h"

#include <algorithm>

namespace Schedule {

QList<int> defaultIntervals()
{
    return {2, 4, 14, 35};
}

QList<Due> due(const QList<Candidate> &candidates, const QHash<QString, PracticeLog::Rating> &latest,
               const QHash<QString, QDate> &firstRated, const QDate &today, const QList<int> &intervals,
               int maxRated, int maxNew)
{
    struct Ranked {
        Due due;
        double urgency;
    };
    QList<Ranked> rated;
    QList<Due> fresh;
    for (const Candidate &c : candidates) {
        const auto it = latest.constFind(c.id);
        if (it == latest.constEnd()) {
            if (c.currentGrade) {
                fresh << Due{c.id, 0, 0};
            }
            continue;
        }
        const int level = it->level;
        const int days = int(it->at.toLocalTime().date().daysTo(today));
        if (days == 0) {
            // Practised today: it takes a place of the day's new ones (new in the current grade)
            // or rated ones.
            if (firstRated.value(c.id) == today) {
                if (c.currentGrade) {
                    --maxNew;
                }
            } else {
                --maxRated;
            }
            continue;
        }
        if (days < 0 || level < 1 || level > intervals.size()) {
            continue;           // a clock from the future
        }
        if (days >= intervals.at(level - 1)) {
            rated << Ranked{Due{c.id, level, days}, (5 - level) * 10.0 + days / 10.0};
        }
    }
    std::stable_sort(rated.begin(), rated.end(), [](const Ranked &a, const Ranked &b) { return a.urgency > b.urgency; });
    QList<Due> result;
    for (int i = 0; i < rated.size() && i < maxRated; ++i) {
        result << rated.at(i).due;
    }
    for (int i = 0; i < fresh.size() && i < maxNew; ++i) {
        result << fresh.at(i);
    }
    return result;
}

} // namespace Schedule
