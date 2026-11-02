#pragma once

#include <QObject>
#include <QUrl>
#include <QVariantMap>
#include <QtQml/qqmlregistration.h>

#include "mediastore.h"

// The backup for QML (Einstellungen → Sicherung): everything the user wrote as one ZIP
// (store/backuparchive), and such a file read back. Reading back merges (TableMerge): new rows
// come in, newer ones replace older, nothing on the device gets lost.
//
//   save(url)       ──► mitori-sicherung-2026-11-02.zip
//   inspect(url)    ──► {created, device, ratings: {added}, notes: {added, changed}, …, starts, media}
//   restore()       merges what inspect() read, then restored() (Practice and Courses reload)
class Backup : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(QString errorString READ errorString NOTIFY errorChanged)

public:
    explicit Backup(QObject *parent = nullptr);
    // For tests: files and folders of their own.
    Backup(const QString &practice, const QString &courses, const QString &media, const QString &work,
           QObject *parent = nullptr);
    ~Backup() override;

    QString errorString() const { return m_error; }

    // "mitori-sicherung-2026-11-02.zip"
    Q_INVOKABLE QString fileName() const;
    Q_INVOKABLE bool save(const QUrl &target);
    // Reads a backup and says what restore() would change; {} on failure (errorString).
    Q_INVOKABLE QVariantMap inspect(const QUrl &source);
    Q_INVOKABLE bool restore();
    // Forgets the inspected backup (its copy in the work folder).
    Q_INVOKABLE void discard();

signals:
    void errorChanged();
    void restored();

private:
    enum class Step { Preview, Apply };
    bool merge(Step step, QVariantMap *summary);
    bool setError(const QString &error);

    QString m_practice;
    QString m_courses;
    MediaStore m_media;
    QString m_work;
    QString m_error;
    bool m_inspected = false;
};
