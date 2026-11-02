#pragma once

#include <QDateTime>
#include <QObject>
#include <QUrl>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>
#include <memory>

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
// Days as "YYYY-MM-DD" (a QDate reaches JS as midnight UTC, the day before west of UTC).
//
//   calendar (ICS, cache)  ──►  upcoming      [{uid, title, first, last, place, teacher, …,
//                                              notebook: uid of its notebook or ""}]
//   CourseBook (SQLite)    ──►  notebooks     [{uid, calendarUid, title, first, last, …, entries}]
//                               entries(uid)  [{uid, text, created, changed}]
class Courses : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(int revision READ revision NOTIFY changed)
    Q_PROPERTY(QVariantList upcoming READ upcoming NOTIFY calendarChanged)
    Q_PROPERTY(QVariantList notebooks READ notebooks NOTIFY changed)
    Q_PROPERTY(CalendarState calendarState READ calendarState NOTIFY calendarChanged)
    // When the calendar was last loaded from the network (local time), or invalid.
    Q_PROPERTY(QDateTime calendarLoaded READ calendarLoaded NOTIFY calendarChanged)
    // Why the notebooks could not be opened or the last change not saved; empty when all is well.
    Q_PROPERTY(QString errorString READ errorString NOTIFY errorChanged)

public:
    enum class CalendarState { Empty, Loading, Ready, Failed };
    Q_ENUM(CalendarState)

    explicit Courses(QObject *parent = nullptr);
    // For tests: a database file and a calendar source (file:// or http) of their own.
    Courses(const QString &path, const QUrl &calendar, const QString &cache, QObject *parent = nullptr);
    ~Courses() override;

    int revision() const { return m_revision; }
    QVariantList upcoming() const;
    QVariantList notebooks() const;
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

    Q_INVOKABLE QVariantList entries(const QString &course) const;
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
