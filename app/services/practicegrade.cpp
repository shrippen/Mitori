#include "practicegrade.h"

#include "courses.h"
#include "programme.h"

#include <QQmlEngine>
#include <QSettings>

namespace {

const QString kChosen = QStringLiteral("practiceGrade/chosen/");
const QString kAgainst = QStringLiteral("practiceGrade/against/");
// Until the split, the one grade of programme page, practice and exam.
const QString kOldGrade = QStringLiteral("programme/grade/");

QString idOf(const QVariant &item)
{
    return item.toMap().value(QStringLiteral("id")).toString();
}

} // namespace

PracticeGrade *PracticeGrade::create(QQmlEngine *engine, QJSEngine *)
{
    return new PracticeGrade(engine->singletonInstance<Programme *>("Mitori", "Programme"),
                             engine->singletonInstance<Courses *>("Mitori", "Courses"));
}

PracticeGrade::PracticeGrade(Programme *programme, Courses *courses, QObject *parent)
    : QObject(parent)
    , m_programme(programme)
    , m_courses(courses)
{
    migrate();
    readNextGrades();

    // Every note changes Courses too: only a new next grade counts.
    connect(m_courses, &Courses::changed, this, [this] {
        const QHash<QString, QString> before = m_nextGrades;
        readNextGrades();
        if (m_nextGrades != before) {
            ++m_revision;
            emit changed();
        }
    });
}

// The old shared value becomes the choice, once, so nobody's practice moves on the update. It is
// held against today's suggestion: only a grade passed from now on moves it.
void PracticeGrade::migrate()
{
    QSettings settings;
    for (const QVariant &d : m_programme->disciplines()) {
        const QString discipline = idOf(d);
        if (!settings.contains(kOldGrade + discipline)) {
            continue;
        }

        const QString old = settings.value(kOldGrade + discipline).toString();
        settings.remove(kOldGrade + discipline);
        const QStringList grades = available(discipline);
        if (!grades.contains(old)) {
            continue;
        }
        settings.setValue(kChosen + discipline, old);
        settings.setValue(kAgainst + discipline, suggested(discipline, grades));
    }
}

void PracticeGrade::readNextGrades()
{
    for (const QVariant &d : m_programme->disciplines()) {
        m_nextGrades.insert(idOf(d), m_courses->nextGrade(idOf(d)));
    }
}

QStringList PracticeGrade::available(const QString &discipline) const
{
    QStringList result;
    for (const QVariant &g : m_programme->grades(discipline)) {
        result << idOf(g);
    }
    return result;
}

// The grade after the highest one passed. One without material (Hon-Menkyo), or none after the
// last: the last with material before it. Nothing before it: the first with material.
QString PracticeGrade::suggested(const QString &discipline, const QStringList &available) const
{
    const QString next = m_courses->nextGrade(discipline);
    if (available.contains(next)) {
        return next;
    }

    QString before;
    for (const QVariant &d : m_programme->disciplines()) {
        if (idOf(d) != discipline) {
            continue;
        }
        for (const QVariant &g : d.toMap().value(QStringLiteral("grade")).toList()) {
            if (idOf(g) == next) {
                break;
            }
            if (available.contains(idOf(g))) {
                before = idOf(g);
            }
        }
    }
    return before.isEmpty() ? available.value(0) : before;
}

// The choice that still holds, or "": gone from the catalog, or overtaken by a grade passed since.
QString PracticeGrade::held(const QString &discipline, const QStringList &available, const QString &suggestion) const
{
    const QSettings settings;
    const QString chosen = settings.value(kChosen + discipline).toString();
    if (!available.contains(chosen)) {
        return {};
    }

    const qsizetype now = available.indexOf(suggestion);
    const qsizetype then = available.indexOf(settings.value(kAgainst + discipline).toString());
    return now > then && now > available.indexOf(chosen) ? QString() : chosen;
}

QString PracticeGrade::grade(const QString &discipline) const
{
    const QStringList grades = available(discipline);
    const QString suggestion = suggested(discipline, grades);
    const QString chosen = held(discipline, grades, suggestion);
    return chosen.isEmpty() ? suggestion : chosen;
}

bool PracticeGrade::manual(const QString &discipline) const
{
    const QStringList grades = available(discipline);
    return !held(discipline, grades, suggested(discipline, grades)).isEmpty();
}

void PracticeGrade::setGrade(const QString &discipline, const QString &grade)
{
    const QStringList grades = available(discipline);
    if (!grades.contains(grade)) {
        return;
    }

    const QString before = this->grade(discipline);
    const bool wasManual = manual(discipline);
    const QString suggestion = suggested(discipline, grades);
    QSettings settings;
    if (grade == suggestion) {
        settings.remove(kChosen + discipline);
        settings.remove(kAgainst + discipline);
    } else {
        settings.setValue(kChosen + discipline, grade);
        settings.setValue(kAgainst + discipline, suggestion);
    }

    if (grade != before || manual(discipline) != wasManual) {
        ++m_revision;
        emit changed();
    }
}
