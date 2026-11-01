#include "practice.h"

#include <QDir>
#include <QSettings>
#include <QStandardPaths>
#include <QUuid>

Practice::Practice(QObject *parent)
    : Practice(defaultPath(), parent)
{
}

Practice::Practice(const QString &path, QObject *parent)
    : QObject(parent)
    , m_log(std::make_unique<PracticeLog>())
{
    load(path);
}

Practice::~Practice() = default;

void Practice::load(const QString &path)
{
    QDir().mkpath(QFileInfo(path).absolutePath());
    if (!m_log->open(path)) {
        m_error = tr("The practice log could not be opened: %1").arg(m_log->errorString());
        qWarning("Practice: %s", qPrintable(m_error));
        return;
    }
    m_open = true;
    m_latest = m_log->latest();
}

QString Practice::defaultPath()
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QStringLiteral("/mitori.sqlite");
}

QString Practice::deviceId()
{
    QSettings settings;
    QString id = settings.value(QStringLiteral("device/id")).toString();
    if (id.isEmpty()) {
        id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        settings.setValue(QStringLiteral("device/id"), id);
    }
    return id;
}

QStringList Practice::levelNames() const
{
    return {tr("not rated"), tr("seen"), tr("with help"), tr("sure"), tr("exam-ready")};
}

bool Practice::rate(const QString &entry, int level)
{
    if (!m_open || entry.isEmpty() || level < 1 || level > 4) {
        return false;
    }
    const PracticeLog::Rating r = m_log->add(entry, level, QDateTime::currentDateTimeUtc(), deviceId());
    if (r.uid.isEmpty()) {
        m_error = tr("The rating could not be saved: %1").arg(m_log->errorString());
        qWarning("Practice: %s", qPrintable(m_error));
        emit errorChanged();
        return false;
    }
    m_latest.insert(entry, r);
    ++m_revision;
    emit changed();
    if (!m_error.isEmpty()) {
        m_error.clear();
        emit errorChanged();
    }
    return true;
}

int Practice::level(const QString &entry) const
{
    return m_latest.value(entry).level;
}

QDateTime Practice::lastRated(const QString &entry) const
{
    const auto it = m_latest.constFind(entry);
    return it == m_latest.constEnd() ? QDateTime() : it->at.toLocalTime();
}

QVariantList Practice::counts(const QStringList &entries) const
{
    QList<int> n(5, 0);
    for (const QString &e : entries) {
        ++n[level(e)];
    }
    return {n[0], n[1], n[2], n[3], n[4]};
}
