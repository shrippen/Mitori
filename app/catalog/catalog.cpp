#include "catalog.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

const Catalog &Catalog::bundled()
{
    static const Catalog catalog = [] {
        Catalog c;
        c.load(QString::fromLatin1(kResource));
        return c;
    }();
    return catalog;
}

bool Catalog::load(const QString &path)
{
    m_disciplines.clear();
    m_videos.clear();
    m_glossary.clear();
    m_glossarySource.clear();
    m_dataVersion.clear();
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return fail(QStringLiteral("%1: %2").arg(path, file.errorString()));
    }
    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (doc.isNull()) {
        return fail(QStringLiteral("%1: %2").arg(path, parseError.errorString()));
    }
    const QJsonObject root = doc.object();
    const int format = root.value(QLatin1String("format")).toInt();
    if (format != kFormat) {
        return fail(QStringLiteral("%1: format %2, expected %3").arg(path).arg(format).arg(kFormat));
    }
    const QJsonArray disciplines = root.value(QLatin1String("disziplinen")).toArray();
    if (disciplines.isEmpty()) {
        return fail(QStringLiteral("%1: no disciplines").arg(path));
    }
    const QString dataVersion = root.value(QLatin1String("stand")).toString();
    if (dataVersion.isEmpty()) {
        return fail(QStringLiteral("%1: no data version").arg(path));
    }
    m_dataVersion = dataVersion;
    m_disciplines = disciplines.toVariantList();
    m_videos = root.value(QLatin1String("videos")).toObject().toVariantMap();
    const QJsonObject glossary = root.value(QLatin1String("glossar")).toObject();
    m_glossary = glossary.value(QLatin1String("woerter")).toArray().toVariantList();
    m_glossarySource = glossary.value(QLatin1String("quelle")).toString();
    m_error.clear();
    return true;
}

bool Catalog::fail(const QString &error)
{
    m_error = error;
    return false;
}
