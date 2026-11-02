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

    // A note on a technique. Changes and deletions keep the uid and set `changed`, so devices can
    // be merged note by note later; a deleted note stays as a mark (`deleted`).
    struct Note {
        QString uid;
        QString entry;
        QString text;
        QDateTime created;      // UTC
        QDateTime changed;      // UTC
        QString device;
        bool deleted = false;
    };

    // A round of the exam mode (mock exam): when, which grade, how many points were asked and
    // the average of their ratings (1..4). The ratings themselves go into `ratings` as usual.
    struct ExamRound {
        QString uid;
        QDateTime at;           // UTC
        QString discipline;     // "aikido", "daitoryu", "shinkitoho"
        QString grade;          // grade id, e.g. "k5"
        int count = 0;
        double average = 0;
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
    // How many entries were rated on each local day from `from` (UTC) on; an entry rated several
    // times on a day counts once (statistics: practice days).
    QHash<QDate, int> practisedPerDay(const QDateTime &from) const;
    // All ratings of one entry, oldest first.
    QList<Rating> history(const QString &entry) const;

    // Notes: add one (an empty uid on failure), change or delete one by uid (false on failure).
    Note addNote(const QString &entry, const QString &text, const QDateTime &at, const QString &device);
    bool changeNote(const QString &uid, const QString &text, const QDateTime &at);
    bool deleteNote(const QString &uid, const QDateTime &at);
    // The notes of one entry that are not deleted, oldest first.
    QList<Note> notes(const QString &entry) const;
    // Every note that is not deleted, newest first.
    QList<Note> allNotes() const;
    // The newest note of every entry that has one.
    QHash<QString, Note> lastNotes() const;

    // A saved exam round with the ratings it wrote.
    struct SavedRound {
        ExamRound round;        // empty uid on failure (errorString())
        QList<Rating> ratings;
    };

    // Exam rounds: add one with its ratings (entry and level each; time and device from the
    // round) in one transaction, all or nothing; the last `limit` rounds of a discipline,
    // oldest first.
    SavedRound addRound(const ExamRound &round, const QList<Rating> &ratings);
    QList<ExamRound> rounds(const QString &discipline, int limit) const;

    static constexpr int schemaVersion = 3;

private:
    bool migrate();

    QString m_connection;
    QString m_error;
};
