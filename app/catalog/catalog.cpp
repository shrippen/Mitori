#include "catalog.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

Catalog::Catalog(QObject *parent)
    : QObject(parent)
{
    load(QString::fromLatin1(kResource));
}

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
    if (root.value(QLatin1String("format")).toInt() != kFormat) {
        return fail(QStringLiteral("%1: format %2, expected %3")
                        .arg(path).arg(root.value(QLatin1String("format")).toInt()).arg(kFormat));
    }
    m_disciplines = root.value(QLatin1String("disziplinen")).toArray().toVariantList();
    m_error.clear();
    emit loaded();
    return true;
}

bool Catalog::fail(const QString &error)
{
    m_error = error;
    emit loaded();
    return false;
}
