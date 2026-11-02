#include "practice.h"

#include "searchkey.h"
#include "schedule.h"

#include <algorithm>

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
    // A minute's check for a new day: it also catches the day after a device slept.
    m_dayTimer.setInterval(60 * 1000);
    connect(&m_dayTimer, &QTimer::timeout, this, [this] {
        if (QDate::currentDate() != m_today) {
            m_today = QDate::currentDate();
            emit todayChanged();
        }
    });
    m_dayTimer.start();
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
    m_first = m_log->firstRated();
    m_lastNotes = m_log->lastNotes();
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
    if (!m_first.contains(entry)) {
        m_first.insert(entry, r.at.toLocalTime().date());
    }
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

bool Practice::ratedToday(const QString &entry) const
{
    const QDateTime last = lastRated(entry);
    return last.isValid() && last.date() == m_today;
}

QVariantList Practice::intervals() const
{
    // Numbers (the settings file gives them back as text after a restart).
    const QVariantList stored = QSettings().value(QStringLiteral("practice/intervals")).toList();
    if (stored.size() == 4) {
        QVariantList days;
        for (const QVariant &v : stored) {
            days << v.toInt();
        }
        return days;
    }
    QVariantList result;
    for (int days : Schedule::defaultIntervals()) {
        result << days;
    }
    return result;
}

void Practice::setIntervals(const QVariantList &intervals)
{
    if (intervals.size() != 4 || intervals == this->intervals()) {
        return;
    }
    QVariantList clean;
    for (const QVariant &v : intervals) {
        clean << qBound(1, v.toInt(), 365);
    }
    QSettings().setValue(QStringLiteral("practice/intervals"), clean);
    emit intervalsChanged();
}

QVariantList Practice::due(const QVariantList &candidates) const
{
    QList<Schedule::Candidate> list;
    QHash<QString, QVariantMap> rows;
    for (const QVariant &c : candidates) {
        const QVariantMap row = c.toMap();
        const QString id = row.value(QStringLiteral("id")).toString();
        list << Schedule::Candidate{id, row.value(QStringLiteral("current")).toBool()};
        rows.insert(id, row);
    }
    QList<int> days;
    for (const QVariant &v : intervals()) {
        days << v.toInt();
    }
    QVariantList result;
    for (const Schedule::Due &d : Schedule::due(list, m_latest, m_first, m_today, days)) {
        QVariantMap row = rows.value(d.id);
        row[QStringLiteral("level")] = d.level;
        row[QStringLiteral("days")] = d.days;
        result << row;
    }
    return result;
}

namespace {

QVariantMap noteMap(const PracticeLog::Note &n)
{
    return {{QStringLiteral("uid"), n.uid},
            {QStringLiteral("text"), n.text},
            {QStringLiteral("created"), n.created.toLocalTime()},
            {QStringLiteral("changed"), n.changed.toLocalTime()}};
}

} // namespace

QVariantList Practice::notes(const QString &entry) const
{
    QVariantList result;
    if (!m_open) {
        return result;
    }
    for (const PracticeLog::Note &n : m_log->notes(entry)) {
        result << noteMap(n);
    }
    return result;
}

QVariantList Practice::searchNotes(const QString &query, int limit) const
{
    QVariantList result;
    const QStringList words = searchWords(query);
    if (!m_open || words.isEmpty()) {
        return result;
    }
    for (const PracticeLog::Note &n : m_log->allNotes()) {
        if (SearchText(n.text).matches(words)) {
            QVariantMap row = noteMap(n);
            row[QStringLiteral("entry")] = n.entry;
            result << row;
            if (result.size() >= limit) {
                break;
            }
        }
    }
    return result;
}

QVariantMap Practice::lastNote(const QString &entry) const
{
    const auto it = m_lastNotes.constFind(entry);
    return it == m_lastNotes.constEnd() ? QVariantMap() : noteMap(*it);
}

void Practice::noteSaved(bool ok)
{
    if (!ok) {
        m_error = tr("The note could not be saved: %1").arg(m_log->errorString());
        qWarning("Practice: %s", qPrintable(m_error));
        emit errorChanged();
        return;
    }
    ++m_notesRevision;
    emit notesChanged();
    if (!m_error.isEmpty()) {
        m_error.clear();
        emit errorChanged();
    }
}

bool Practice::addNote(const QString &entry, const QString &text)
{
    if (!m_open || entry.isEmpty() || text.trimmed().isEmpty()) {
        return false;
    }
    const PracticeLog::Note n = m_log->addNote(entry, text, QDateTime::currentDateTimeUtc(), deviceId());
    if (!n.uid.isEmpty()) {
        m_lastNotes.insert(entry, n);
    }
    noteSaved(!n.uid.isEmpty());
    return !n.uid.isEmpty();
}

bool Practice::changeNote(const QString &uid, const QString &text)
{
    if (!m_open || text.trimmed().isEmpty()) {
        return false;
    }
    const bool ok = m_log->changeNote(uid, text, QDateTime::currentDateTimeUtc());
    if (ok) {
        m_lastNotes = m_log->lastNotes();
    }
    noteSaved(ok);
    return ok;
}

bool Practice::deleteNote(const QString &uid)
{
    if (!m_open) {
        return false;
    }
    const bool ok = m_log->deleteNote(uid, QDateTime::currentDateTimeUtc());
    if (ok) {
        m_lastNotes = m_log->lastNotes();
    }
    noteSaved(ok);
    return ok;
}
