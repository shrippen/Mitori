#pragma once

#include <QDate>
#include <QDateTime>
#include <QHash>
#include <QList>
#include <QString>
#include <QStringList>

// Course notebooks and the user's grades in SQLite (a file of their own next to the practice log):
// a notebook per course with its entries, each with a time stamp and the device it came from, so that devices can later
// be merged entry by entry (sync). Changes and deletions keep the uid and set `changed`; a deleted
// row stays as a mark (`deleted`). Plain C++; QML reaches it through the Courses service.
//
//   courses   uid, calendar_uid (the ICS event's, "" for one of the user's own), title, first,
//             last, place, teacher, info, announcement, created, changed, device, deleted,
//             exam_discipline, exam_grade (an exam planned there, "" for none; schema 2)
//   entries   uid, course, text, created, changed, device, deleted, media (schema 3)
//   grades    uid, discipline, grade, day, place, created, changed, device, deleted (schema 2)
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
        QString examDiscipline; // the exam planned on this course, or ""
        QString examGrade;
    };

    struct Entry {
        QString uid;
        QString course;         // the course's uid
        QString text;
        QDateTime created;      // UTC
        QDateTime changed;      // UTC
        QString device;
        bool deleted = false;
        QString media;          // a photo or voice note: its file in the media folder, or ""
    };

    // A grade the user passed (Meine Grade): discipline and grade ids of the catalog.
    struct Grade {
        QString uid;
        QString discipline;
        QString grade;
        QDate day;
        QString place;
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
    // Plans an exam on a course; an empty discipline or grade removes it.
    bool planExam(const QString &uid, const QString &discipline, const QString &grade, const QDateTime &at);

    // Entries of a notebook: add one (an empty uid on failure), change or delete one by uid.
    // Text, a media file (MediaStore), or both.
    Entry addEntry(const QString &course, const QString &text, const QDateTime &at, const QString &device,
                   const QString &media = {});
    // The media file of an entry, and of every entry of a notebook (also deleted ones).
    QString entryMedia(const QString &uid) const;
    QStringList courseMedia(const QString &course) const;
    bool changeEntry(const QString &uid, const QString &text, const QDateTime &at);
    bool deleteEntry(const QString &uid, const QDateTime &at);
    // The entries of a notebook that are not deleted, oldest first.
    QList<Entry> entries(const QString &course) const;
    // The entries of every notebook that is not deleted, newest first (All notes).
    QList<Entry> allEntries() const;
    // How many entries each notebook has: {course uid: count}.
    QHash<QString, int> entryCounts() const;

    // Passed grades: one per discipline and grade (adding it again replaces the day and place);
    // an empty uid on failure.
    Grade addGrade(Grade grade, const QDateTime &at, const QString &device);
    bool deleteGrade(const QString &uid, const QDateTime &at);
    // Every passed grade that is not deleted, the latest day first.
    QList<Grade> grades() const;

    static constexpr int schemaVersion = 3;

private:
    bool migrate();
    bool mark(const QString &table, const QString &uid, const QDateTime &at);

    QString m_connection;
    QString m_error;
};
