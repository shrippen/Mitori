#pragma once

#include <QObject>
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
    // Why the log could not be opened or the last rating not saved; empty when all is well.
    Q_PROPERTY(QString errorString READ errorString NOTIFY errorChanged)
    // Words per level: [none, seen, with help, sure, ready for the exam] (translated).
    Q_PROPERTY(QStringList levelNames READ levelNames CONSTANT)

public:
    explicit Practice(QObject *parent = nullptr);
    // For tests: a database file of its own.
    explicit Practice(const QString &path, QObject *parent = nullptr);
    ~Practice() override;

    int revision() const { return m_revision; }
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

    static QString defaultPath();
    // This device's id for the log (made once, kept in the settings).
    static QString deviceId();

signals:
    void changed();
    void errorChanged();

private:
    void load(const QString &path);

    std::unique_ptr<PracticeLog> m_log;
    QHash<QString, PracticeLog::Rating> m_latest;
    QString m_error;
    bool m_open = false;
    int m_revision = 0;
};
