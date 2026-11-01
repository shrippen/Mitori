#include "catalog.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

bool Catalog::load(const QString &path)
{
    m_disciplines.clear();
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
    m_disciplines = disciplines.toVariantList();
    m_error.clear();
    return true;
}

bool Catalog::fail(const QString &error)
{
    m_error = error;
    return false;
}
