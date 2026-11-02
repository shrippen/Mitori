#pragma once

#include <QByteArray>
#include <QDateTime>
#include <QList>
#include <QString>
#include <QUrl>

class QNetworkAccessManager;
class QNetworkReply;

// A small WebDAV client for the sync (Nextcloud, ownCloud, any WebDAV server): folders, listing,
// files up and down. Each call waits for its answer (the sync runs on a thread of its own) and
// gives up after a timeout. Paths are relative to the account's folder.
//
//   https://cloud.example.org/remote.php/dav/files/ich/Mitori/   ← folder
//     geraete/3f2a….zip                                          ← list("geraete"), get, put
class WebDav
{
public:
    struct Account {
        QUrl folder;            // ends in "/"
        QString user;
        QString password;       // an app password
    };

    struct Item {
        QString name;
        QDateTime modified;     // UTC
        qint64 size = 0;
        bool folder = false;
    };

    explicit WebDav(const Account &account);
    ~WebDav();
    WebDav(const WebDav &) = delete;
    WebDav &operator=(const WebDav &) = delete;

    QString errorString() const { return m_error; }

    // Makes a folder unless it is there.
    bool ensureFolder(const QString &path);
    // The files and folders in a folder (not the folder itself).
    bool list(const QString &path, QList<Item> *items);
    bool get(const QString &path, QByteArray *data);
    bool put(const QString &path, const QByteArray &data);

    static constexpr int kTimeoutMs = 60 * 1000;

private:
    // A request and its status code (0 when there was no answer); the body in `answer`.
    int send(const QByteArray &verb, const QString &path, const QByteArray &body, QByteArray *answer,
             const QList<std::pair<QByteArray, QByteArray>> &headers = {});
    QUrl url(const QString &path) const;
    bool fail(int status, const QByteArray &verb, const QString &path);

    Account m_account;
    QNetworkAccessManager *m_net;
    QString m_error;
    QString m_replyError;
};
