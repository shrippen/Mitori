#pragma once

#include <QDate>
#include <QObject>
#include <QTimer>
#include <QStringList>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>
#include <memory>

#include "practicelog.h"

// How well each technique sits, for QML: rate an entry 1..4, read its latest level and when it
// was rated, count levels for a grade. Backed by the practice log (SQLite in the app's data
// folder). `revision` grows with every change, so bindings that read levels follow.
class Practice : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(int revision READ revision NOTIFY changed)
    // Grows with every new, changed or deleted note.
    Q_PROPERTY(int notesRevision READ notesRevision NOTIFY notesChanged)
    // Why the log could not be opened or the last rating not saved; empty when all is well.
    Q_PROPERTY(QString errorString READ errorString NOTIFY errorChanged)
    // Days until a technique of level 1..4 is due again (settings, "Heute üben").
    Q_PROPERTY(QVariantList intervals READ intervals WRITE setIntervals NOTIFY intervalsChanged)
    // Today's date; changes at midnight, so due lists and "rated today" follow a new day.
    Q_PROPERTY(QDate today READ today NOTIFY todayChanged)
    // Words per level: [none, seen, with help, sure, ready for the exam] (translated).
    Q_PROPERTY(QStringList levelNames READ levelNames CONSTANT)

public:
    explicit Practice(QObject *parent = nullptr);
    // For tests: a database file of its own.
    explicit Practice(const QString &path, QObject *parent = nullptr);
    ~Practice() override;

    int revision() const { return m_revision; }
    int notesRevision() const { return m_notesRevision; }
    QString errorString() const { return m_error; }
    QStringList levelNames() const;

    // A new rating of an entry now (1..4); a rating of the same level again counts as practice.
    // False (and errorString set) when it could not be saved.
    Q_INVOKABLE bool rate(const QString &entry, int level);
    // The latest level of an entry, 0 if never rated.
    Q_INVOKABLE int level(const QString &entry) const;
    // When the entry was rated last (local time), or an invalid date.
    Q_INVOKABLE QDateTime lastRated(const QString &entry) const;
    // How many of the entries stand at each level: [none, 1, 2, 3, 4].
    Q_INVOKABLE QVariantList counts(const QStringList &entries) const;
    // Whether the entry was rated today (practised).
    Q_INVOKABLE bool ratedToday(const QString &entry) const;
    // Today's practice from candidate rows (Programme.practiceEntries: id, current, …): the due
    // ones in order (Schedule::due), each row with `level` and `days` added.
    Q_INVOKABLE QVariantList due(const QVariantList &candidates) const;

    // Notes on a technique: {uid, text, created, changed} with local dates, oldest first.
    Q_INVOKABLE QVariantList notes(const QString &entry) const;
    // The newest note of an entry ({uid, text, created, changed}), or an empty map.
    Q_INVOKABLE QVariantMap lastNote(const QString &entry) const;
    // Add, change or delete a note; false (and errorString set) when it could not be saved.
    Q_INVOKABLE bool addNote(const QString &entry, const QString &text);
    Q_INVOKABLE bool changeNote(const QString &uid, const QString &text);
    Q_INVOKABLE bool deleteNote(const QString &uid);

    QDate today() const { return m_today; }
    QVariantList intervals() const;
    void setIntervals(const QVariantList &intervals);

    static QString defaultPath();
    // This device's id for the log (made once, kept in the settings).
    static QString deviceId();

signals:
    void changed();
    void errorChanged();
    void intervalsChanged();
    void todayChanged();
    void notesChanged();

private:
    void load(const QString &path);
    void noteSaved(bool ok);

    std::unique_ptr<PracticeLog> m_log;
    QHash<QString, PracticeLog::Rating> m_latest;
    QHash<QString, QDate> m_first;
    QHash<QString, PracticeLog::Note> m_lastNotes;
    QDate m_today = QDate::currentDate();
    QTimer m_dayTimer;
    QString m_error;
    bool m_open = false;
    int m_revision = 0;
    int m_notesRevision = 0;
};
