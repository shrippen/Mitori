#pragma once

#include <QDateTime>
#include <QObject>
#include <QUrl>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>
#include <memory>

#include "catalog.h"
#include "coursebook.h"
#include "devicecalendar.h"
#include "icsreader.h"

class QNetworkAccessManager;
class QNetworkReply;

// Courses for QML (clickdummy "Lehrgänge"): the coming events of the association's public course
// calendar (ICS, kept on the device for offline use) and the user's notebooks, one per course,
// made from an event or by hand (a private training, a guest teacher). `revision` grows with
// every change, so bindings that read notebooks or entries follow.
//
// Also the user's grades (Meine Grade) and the exams planned on courses: the waiting time before
// the next grade comes from the programme data (`wartezeit` of a grade, tools/daten.py).
//
// Days as "YYYY-MM-DD" (a QDate reaches JS as midnight UTC, the day before west of UTC).
//
//   calendar (ICS, cache)  ──►  upcoming      [{uid, title, first, last, place, teacher, …,
//                                              notebook: uid of its notebook or ""}]
//   CourseBook (SQLite)    ──►  notebooks     [{uid, calendarUid, title, first, last, …, entries}]
//                               entries(uid)  [{uid, text, created, changed}]
//                               grades        [{uid, discipline, grade, name, day, place}]
//   + catalog (grade order, waiting times) ──►  nextGrade, waiting, nextExam
class Courses : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(int revision READ revision NOTIFY changed)
    Q_PROPERTY(QVariantList upcoming READ upcoming NOTIFY calendarChanged)
    Q_PROPERTY(QVariantList notebooks READ notebooks NOTIFY changed)
    // Passed grades, the latest day first: {uid, discipline, grade, name, day, place}.
    Q_PROPERTY(QVariantList grades READ grades NOTIFY changed)
    Q_PROPERTY(CalendarState calendarState READ calendarState NOTIFY calendarChanged)
    // When the calendar was last loaded from the network (local time), or invalid.
    Q_PROPERTY(QDateTime calendarLoaded READ calendarLoaded NOTIFY calendarChanged)
    // Why the notebooks could not be opened or the last change not saved; empty when all is well.
    Q_PROPERTY(QString errorString READ errorString NOTIFY errorChanged)

public:
    enum class CalendarState { Empty, Loading, Ready, Failed };
    Q_ENUM(CalendarState)
    // How a grade's waiting time stands (waiting()):
    //   First      the first grade, no waiting time
    //   FromStart  `years` after the start of training (no day known to the app)
    //   Unknown    `years` after `previous`, which is not entered
    //   Earliest   from `earliest` on (no exam day given)
    //   Fits       the exam day is on or after `earliest`
    //   TooEarly   the exam day is before `earliest`
    enum class Wait { First, FromStart, Unknown, Earliest, Fits, TooEarly };
    Q_ENUM(Wait)

    explicit Courses(QObject *parent = nullptr);
    // For tests: a database file, a calendar source (file:// or http) and a catalog of their own.
    Courses(const QString &path, const QUrl &calendar, const QString &cache, const Catalog &catalog,
            QObject *parent = nullptr);
    ~Courses() override;

    int revision() const { return m_revision; }
    QVariantList upcoming() const;
    QVariantList notebooks() const;
    QVariantList grades() const;
    CalendarState calendarState() const { return m_state; }
    QDateTime calendarLoaded() const { return m_loaded.toLocalTime(); }
    QString errorString() const { return m_error; }

    // Loads the calendar anew; `Older` only when the last load is older than a few hours.
    enum class Reload { Always, Older };
    Q_ENUM(Reload)
    Q_INVOKABLE void reload(Reload when = Reload::Always);

    // The notebook of a calendar event, made on first use; its uid ("" on failure).
    Q_INVOKABLE QString notebookFor(const QString &eventUid);
    // A notebook without a calendar event, days as "YYYY-MM-DD"; its uid ("" on failure).
    Q_INVOKABLE QString create(const QString &title, const QString &first, const QString &last,
                               const QString &place, const QString &teacher);
    Q_INVOKABLE bool remove(const QString &uid);
    // One notebook as in `notebooks`, or an empty map.
    Q_INVOKABLE QVariantMap notebook(const QString &uid) const;

    // A passed grade, day as "YYYY-MM-DD"; the same grade again corrects day and place.
    Q_INVOKABLE bool addGrade(const QString &discipline, const QString &grade, const QString &day, const QString &place);
    Q_INVOKABLE bool removeGrade(const QString &uid);
    // The grades one can take in a discipline, in order: {id, name, passed}.
    Q_INVOKABLE QVariantList ladder(const QString &discipline) const;
    // The grade after the highest one passed (the first without any), or "" after the last.
    Q_INVOKABLE QString nextGrade(const QString &discipline) const;
    // The waiting time before a grade: {state (Wait), years, previous (name), earliest ("YYYY-MM-DD"
    // or "")}; `day` ("YYYY-MM-DD", optional) is the planned exam day it is checked against.
    Q_INVOKABLE QVariantMap waiting(const QString &discipline, const QString &grade, const QString &day = {}) const;

    // Plans an exam on a notebook's course; an empty grade removes it.
    Q_INVOKABLE bool planExam(const QString &uid, const QString &discipline, const QString &grade);
    // The next planned exam whose course has not ended (of a discipline, or of any with ""):
    // {course, title, place, first, discipline, grade, name, days (from today to the first day,
    // 0 while it runs)}, or an empty map.
    Q_INVOKABLE QVariantMap nextExam(const QString &discipline = {}) const;

    Q_INVOKABLE QVariantList entries(const QString &course) const;
    // Every notebook's entries, newest first, and the notebooks' titles by uid (C++ only: AllNotes).
    QList<CourseBook::Entry> allEntries() const;
    QHash<QString, QString> courseTitles() const;
    Q_INVOKABLE bool addEntry(const QString &course, const QString &text);
    Q_INVOKABLE bool changeEntry(const QString &uid, const QString &text);
    Q_INVOKABLE bool deleteEntry(const QString &uid);

    // The course into the device's calendar app (platform/devicecalendar); false without one.
    Q_INVOKABLE bool addToDeviceCalendar(const QString &uid);
    // The event it hands over (tests).
    DeviceCalendar::Event deviceEvent(const QString &uid) const;

    // The teacher from a description's first line "mit …" ("mit M. D. Nakajima Shihan").
    static QString teacherOf(const QString &description);
    static QString defaultPath();

signals:
    void changed();
    void calendarChanged();
    void errorChanged();

private:
    void loadCache();
    void received(QNetworkReply *reply);
    bool saved(bool ok, const QString &what);
    // A grade of the catalog that one can take, in its discipline's order.
    struct Step {
        QString id;
        QString name;
        int years = 0;          // 0: none
        bool fromStart = false; // years after the start of training, not after the previous grade
    };
    QList<Step> steps(const QString &discipline) const;
    QString gradeName(const QString &discipline, const QString &grade) const;
    QString examName(const QString &discipline, const QString &grade) const;
    QVariantMap courseMap(const CourseBook::Course &c, int entries) const;

    QHash<QString, QList<Step>> m_steps;
    QHash<QString, QString> m_disciplineNames;

    std::unique_ptr<CourseBook> m_book;
    QNetworkAccessManager *m_network = nullptr;
    QUrl m_source;
    QString m_cache;
    QList<IcsReader::Event> m_events;
    CalendarState m_state = CalendarState::Empty;
    QDateTime m_loaded;
    QString m_error;
    bool m_open = false;
    int m_revision = 0;
};
