#pragma once

#include <QDate>
#include <QDateTime>
#include <QHash>
#include <QList>
#include <QString>

// Course notebooks in SQLite (a file of their own next to the practice log): a notebook per course
// with its entries, each with a time stamp and the device it came from, so that devices can later
// be merged entry by entry (sync). Changes and deletions keep the uid and set `changed`; a deleted
// row stays as a mark (`deleted`). Plain C++; QML reaches it through the Courses service.
//
//   courses   uid, calendar_uid (the ICS event's, "" for one of the user's own), title, first,
//             last, place, teacher, info, announcement, created, changed, device, deleted
//   entries   uid, course, text, created, changed, device, deleted
class CourseBook
{
public:
    struct Course {
        QString uid;
        QString calendarUid;    // the event of the course calendar it was made from, or ""
        QString title;
        QDate first;
        QDate last;             // inclusive
        QString place;
        QString teacher;
        QString info;           // the event's description
        QString announcement;   // the official announcement (URL), or ""
        QDateTime created;      // UTC
        QDateTime changed;      // UTC
        QString device;
        bool deleted = false;
    };

    struct Entry {
        QString uid;
        QString course;         // the course's uid
        QString text;
        QDateTime created;      // UTC
        QDateTime changed;      // UTC
        QString device;
        bool deleted = false;
    };

    CourseBook();
    ~CourseBook();
    CourseBook(const CourseBook &) = delete;
    CourseBook &operator=(const CourseBook &) = delete;

    // Opens (and creates or migrates) the database file; false with errorString() on failure.
    bool open(const QString &path);
    QString errorString() const { return m_error; }

    // A new notebook (uid, created, changed and device set here); an empty uid on failure.
    Course addCourse(Course course, const QDateTime &at, const QString &device);
    // Every notebook that is not deleted, the latest course first.
    QList<Course> courses() const;
    // The notebook made from a calendar event, or one with an empty uid.
    Course courseForEvent(const QString &calendarUid) const;
    bool deleteCourse(const QString &uid, const QDateTime &at);

    // Entries of a notebook: add one (an empty uid on failure), change or delete one by uid.
    Entry addEntry(const QString &course, const QString &text, const QDateTime &at, const QString &device);
    bool changeEntry(const QString &uid, const QString &text, const QDateTime &at);
    bool deleteEntry(const QString &uid, const QDateTime &at);
    // The entries of a notebook that are not deleted, oldest first.
    QList<Entry> entries(const QString &course) const;
    // How many entries each notebook has: {course uid: count}.
    QHash<QString, int> entryCounts() const;

    static constexpr int schemaVersion = 1;

private:
    bool migrate();
    bool mark(const QString &table, const QString &uid, const QDateTime &at);

    QString m_connection;
    QString m_error;
};
