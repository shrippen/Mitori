#pragma once

#include <QDateTime>
#include <QHash>
#include <QList>
#include <QString>

// The practice log in SQLite: every rating of a technique as its own entry with a time stamp
// and the device it came from, so that devices can later be merged entry by entry (sync) and
// the history stays (statistics, "last rated"). Plain C++; QML reaches it through Practice.
class PracticeLog
{
public:
    struct Rating {
        QString uid;
        QString entry;          // programme id, e.g. "aikido/k5/tachi-waza/shomen-uchi/ikkyo"
        int level = 0;          // 1 seen, 2 with help, 3 sure, 4 ready for the exam
        QDateTime at;           // UTC
        QString device;
    };

    PracticeLog();
    ~PracticeLog();
    PracticeLog(const PracticeLog &) = delete;
    PracticeLog &operator=(const PracticeLog &) = delete;

    // Opens (and creates or migrates) the database file; false with errorString() on failure.
    bool open(const QString &path);
    QString errorString() const { return m_error; }

    // Adds a rating; returns it with its new uid, or an empty uid on failure (errorString()).
    Rating add(const QString &entry, int level, const QDateTime &at, const QString &device);
    // The latest rating of every entry (for lists); entries with a level outside 1..4 are skipped.
    QHash<QString, Rating> latest() const;
    // The local day each entry was rated first (its history begins: it was new that day).
    QHash<QString, QDate> firstRated() const;
    // All ratings of one entry, oldest first.
    QList<Rating> history(const QString &entry) const;

    static constexpr int schemaVersion = 1;

private:
    bool migrate();

    QString m_connection;
    QString m_error;
};
