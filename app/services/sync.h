#pragma once

#include "syncaccount.h"
#include "syncjob.h"

#include <QFutureWatcher>
#include <QObject>
#include <QTimer>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

// The sync between devices over WebDAV / Nextcloud for QML (Einstellungen → Abgleich, clickdummy
// "Sicherung"): the account, "Sync now", the devices and the conflicts to solve. A sync runs on a
// thread of its own (SyncJob); after one that brought something, merged() (Practice and Courses
// read anew). It runs when the app starts, once a day, and after changes when `automatic`.
//
//   conflict   {index, kind: "note"|"entry", parent, mine, theirs, mineChanged, theirsChanged, device}
//   resolve(index, Mine | Theirs | Both)    "Both" keeps the other text as a new note
class Sync : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY accountChanged)
    Q_PROPERTY(bool automatic READ automatic WRITE setAutomatic NOTIFY accountChanged)
    Q_PROPERTY(QString server READ server NOTIFY accountChanged)
    Q_PROPERTY(QString user READ user NOTIFY accountChanged)
    Q_PROPERTY(bool hasPassword READ hasPassword NOTIFY accountChanged)
    // Why the password is not simply in the key store; the settings say so.
    Q_PROPERTY(PasswordNote passwordNote READ passwordNote NOTIFY accountChanged)
    Q_PROPERTY(bool passwordInFile READ passwordInFile NOTIFY accountChanged)
    // It survives a restart as it is (not: known right now).
    Q_PROPERTY(bool passwordSaved READ passwordSaved NOTIFY accountChanged)
    Q_PROPERTY(QString deviceName READ deviceName NOTIFY accountChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(QDateTime lastSync READ lastSync NOTIFY stateChanged)
    Q_PROPERTY(QVariantList devices READ devices NOTIFY stateChanged)
    Q_PROPERTY(QVariantList conflicts READ conflicts NOTIFY stateChanged)
    Q_PROPERTY(QString errorString READ errorString NOTIFY stateChanged)

public:
    enum class Choice { Mine, Theirs, Both };
    Q_ENUM(Choice)

    enum class PasswordNote { NoNote, MovingToStore, RefusedByStore, NotRemovedFromFile, NotRecorded, NotSaved, Unreadable };
    Q_ENUM(PasswordNote)

    explicit Sync(QObject *parent = nullptr);
    // For tests: files, folders and key store of their own, no timers.
    Sync(const QString &practice, const QString &courses, const QString &media, const QString &folder,
         std::shared_ptr<SecretStore> store, QObject *parent = nullptr);
    ~Sync() override;

    bool enabled() const { return m_account.enabled(); }
    void setEnabled(bool on);
    bool automatic() const { return m_account.automatic(); }
    void setAutomatic(bool on);
    QString server() const { return m_account.account().folder.toString(); }
    QString user() const { return m_account.account().user; }
    bool hasPassword() const { return m_account.secret() != SyncAccount::Secret::None; }
    PasswordNote passwordNote() const;
    bool passwordSaved() const { return m_account.passwordSaved(); }
    bool passwordInFile() const;
    bool passwordUnreadable() const { return m_account.secret() == SyncAccount::Secret::Unreadable; }
    QString deviceName() const;
    bool busy() const { return m_busy; }
    QDateTime lastSync() const { return m_account.last(); }
    QVariantList devices() const;
    QVariantList conflicts() const;
    QString errorString() const { return m_error; }

    // The folder on the server (https://…/remote.php/dav/files/<user>/Mitori/), the user and an
    // app password ("" keeps the saved one), this device's name.
    Q_INVOKABLE void setAccount(const QString &server, const QString &user, const QString &password,
                                const QString &name);
    // Starts a sync unless one runs (or once the key store answered); false when there is no account.
    Q_INVOKABLE bool syncNow();
    // A change was made: with `automatic`, a sync follows soon.
    Q_INVOKABLE void touched();
    Q_INVOKABLE bool resolve(int index, Choice choice);

    // Waits for a running sync (tests).
    void wait();

    static constexpr int kStartDelayMs = 5 * 1000;
    static constexpr int kAfterChangeMs = 30 * 1000;
    static constexpr int kDailyMs = 24 * 60 * 60 * 1000;

signals:
    void accountChanged();
    void busyChanged();
    void stateChanged();
    void merged();

private slots:
    // What the timers start. Not the button: a store that refused is asked again only by it, not
    // with a prompt every 30 s.
    void scheduled();

private:
    void finished();
    void setError(const QString &error);
    void announce();
    void secretFound();

    QString m_practice;
    QString m_courses;
    QString m_media;
    QString m_work;
    SyncAccount m_account;
    QFutureWatcher<SyncJob::Output> m_watcher;
    QTimer m_soon;
    QTimer m_daily;
    QString m_error;
    bool m_busy = false;
    bool m_merging = false;     // while merged() runs: the services reading anew are no change
    bool m_again = false;       // asked for while one ran
    bool m_waiting = false;     // asked for before the key store answered
};
