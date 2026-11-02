#include "webdav.h"

#include <QCoreApplication>
#include <QEventLoop>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <QXmlStreamReader>

namespace {

// HTTP and WebDAV status codes (RFC 9110, RFC 4918).
constexpr int kOk = 200;
constexpr int kCreated = 201;
constexpr int kNoContent = 204;
constexpr int kMultiStatus = 207;
constexpr int kUnauthorized = 401;
constexpr int kNotAllowed = 405;            // MKCOL on a folder that is there

const QByteArray kPropfind =
    "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
    "<d:propfind xmlns:d=\"DAV:\"><d:prop><d:resourcetype/><d:getlastmodified/><d:getcontentlength/></d:prop>"
    "</d:propfind>";

QString trimmed(QString path)
{
    while (path.endsWith(QLatin1Char('/'))) {
        path.chop(1);
    }
    return path;
}

} // namespace

WebDav::WebDav(const Account &account)
    : m_account(account)
    , m_net(new QNetworkAccessManager)
{
}

WebDav::~WebDav()
{
    delete m_net;
}

QUrl WebDav::url(const QString &path) const
{
    QUrl base = m_account.folder;
    if (!base.path().endsWith(QLatin1Char('/'))) {
        base.setPath(base.path() + QLatin1Char('/'));
    }
    // Relative to the folder: no leading "/" (that would be the server's root).
    QString relative = path;
    while (relative.startsWith(QLatin1Char('/'))) {
        relative.remove(0, 1);
    }
    return base.resolved(QUrl(QString::fromLatin1(QUrl::toPercentEncoding(relative, "/"))));
}

int WebDav::send(const QByteArray &verb, const QString &path, const QByteArray &body, QByteArray *answer,
                 const QList<std::pair<QByteArray, QByteArray>> &headers)
{
    QNetworkRequest request(url(path));
    request.setRawHeader("Authorization",
                         "Basic " + (m_account.user + QLatin1Char(':') + m_account.password).toUtf8().toBase64());
    for (const auto &[name, value] : headers) {
        request.setRawHeader(name, value);
    }
    request.setTransferTimeout(kTimeoutMs);

    QNetworkReply *reply = m_net->sendCustomRequest(request, verb, body);
    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    m_replyError = reply->errorString();
    if (answer) {
        *answer = reply->readAll();
    }
    reply->deleteLater();
    return status;
}

bool WebDav::fail(int status, const QByteArray &verb, const QString &path)
{
    if (status == kUnauthorized) {
        m_error = QCoreApplication::translate("WebDav", "the server did not accept the user name or app password");
    } else if (status == 0) {
        m_error = m_replyError;
    } else {
        m_error = QCoreApplication::translate("WebDav", "%1 %2: the server answered %3")
                      .arg(QString::fromLatin1(verb), path.isEmpty() ? QStringLiteral("/") : path)
                      .arg(status);
    }
    return false;
}

bool WebDav::ensureFolder(const QString &path)
{
    const int status = send("MKCOL", path + QLatin1Char('/'), {}, nullptr);
    if (status == kCreated || status == kNotAllowed) {
        return true;
    }
    return fail(status, "MKCOL", path);
}

bool WebDav::list(const QString &path, QList<Item> *items)
{
    items->clear();
    QByteArray answer;
    const int status = send("PROPFIND", path + QLatin1Char('/'), kPropfind, &answer,
                            {{"Depth", "1"}, {"Content-Type", "application/xml; charset=utf-8"}});
    if (status != kMultiStatus) {
        return fail(status, "PROPFIND", path);
    }

    // One <response> per item; the first is the folder itself.
    const QString self = trimmed(url(path).path());
    QXmlStreamReader xml(answer);
    Item item;
    QString href;
    while (!xml.atEnd()) {
        xml.readNext();
        const QStringView name = xml.name();
        if (xml.isStartElement() && name == u"response") {
            item = {};
            href.clear();
        } else if (xml.isStartElement() && name == u"href") {
            href = trimmed(QUrl(xml.readElementText()).path());
        } else if (xml.isStartElement() && name == u"getlastmodified") {
            item.modified = QDateTime::fromString(xml.readElementText(), Qt::RFC2822Date).toUTC();
        } else if (xml.isStartElement() && name == u"getcontentlength") {
            item.size = xml.readElementText().toLongLong();
        } else if (xml.isStartElement() && name == u"collection") {
            item.folder = true;
        } else if (xml.isEndElement() && name == u"response" && !href.isEmpty() && href != self) {
            item.name = href.section(QLatin1Char('/'), -1);
            *items << item;
        }
    }
    if (xml.hasError()) {
        m_error = xml.errorString();
        return false;
    }
    return true;
}

bool WebDav::get(const QString &path, QByteArray *data)
{
    const int status = send("GET", path, {}, data);
    if (status != kOk) {
        data->clear();
        return fail(status, "GET", path);
    }
    return true;
}

bool WebDav::put(const QString &path, const QByteArray &data)
{
    const int status = send("PUT", path, data, nullptr, {{"Content-Type", "application/octet-stream"}});
    if (status == kOk || status == kCreated || status == kNoContent) {
        return true;
    }
    return fail(status, "PUT", path);
}
