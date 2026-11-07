#include "demo.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QQmlContext>
#include <QQmlExpression>
#include <QQuickWindow>
#include <QSaveFile>
#include <QSettings>
#include <QTimer>
#include <QUrl>

#include "appearance.h"
#include "coursebook.h"
#include "courses.h"
#include "practice.h"
#include "practicelog.h"

namespace {

// The window of a shot without its own `viewport`: a phone.
constexpr int kPhoneWidth = 420;
constexpr int kPhoneHeight = 860;
// After each step of a shot: the page slides in, lists fill. Before the picture twice that: a
// page pushed first is compiled first (the programme takes longer than one step).
constexpr int kSettleMs = 2500;

const QHash<QString, Appearance::Style> kStyles{{QStringLiteral("breeze"), Appearance::Style::Breeze},
                                                {QStringLiteral("material"), Appearance::Style::Material},
                                                {QStringLiteral("kante"), Appearance::Style::Kante},
                                                {QStringLiteral("leinen"), Appearance::Style::Leinen},
                                                {QStringLiteral("shinki"), Appearance::Style::Shinki}};

QString language()
{
    return qEnvironmentVariable("DEMO_LANG", QStringLiteral("de"));
}

// A world text: {"de": …, "en": …} in the demo's language, or a plain string.
QString text(const QJsonValue &value)
{
    if (value.isObject()) {
        return value.toObject().value(language()).toString();
    }
    return value.toString();
}

// A day relative to today (0 = today) at a local time ("19:30"), in UTC as the stores keep it.
QDateTime at(int day, const QString &time = QStringLiteral("12:00"))
{
    return QDateTime(QDate::currentDate().addDays(day), QTime::fromString(time, QStringLiteral("HH:mm"))).toUTC();
}

QDate dayOf(const QJsonValue &value)
{
    return QDate::currentDate().addDays(value.toInt());
}

QJsonObject dojo()
{
    QFile file(qEnvironmentVariable("MITORI_DEMO_WORLD"));
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning("Demo: no world at MITORI_DEMO_WORLD (%s)", qPrintable(file.fileName()));
        return {};
    }
    return QJsonDocument::fromJson(file.readAll()).object().value(QStringLiteral("dojo")).toObject();
}

// What the welcome wizard and Appearance would have saved.
void writeSettings()
{
    QSettings settings;
    settings.setValue(QStringLiteral("look/welcomed"), true);
    const Appearance::Language lang = language() == QLatin1String("en") ? Appearance::Language::English : Appearance::Language::German;
    settings.setValue(QStringLiteral("look/language"), int(lang));
    const QString style = qEnvironmentVariable("DEMO_STYLE", QStringLiteral("leinen"));
    settings.setValue(QStringLiteral("look/style"), int(kStyles.value(style, Appearance::Style::Leinen)));
    settings.setValue(QStringLiteral("programme/discipline"), QStringLiteral("aikido"));
}

// Ratings, notes and mock exams.
void seedPractice(const QJsonObject &world, const QString &device)
{
    PracticeLog log;
    if (!log.open(Practice::defaultPath())) {
        qWarning("Demo: %s", qPrintable(log.errorString()));
        return;
    }

    // [day, time, entry, level]
    for (const QJsonValue &v : world.value(QStringLiteral("ratings")).toArray()) {
        const QJsonArray r = v.toArray();
        log.add(r.at(2).toString(), r.at(3).toInt(), at(r.at(0).toInt(), r.at(1).toString()), device);
    }

    for (const QJsonValue &v : world.value(QStringLiteral("notes")).toArray()) {
        const QJsonObject n = v.toObject();
        log.addNote(n.value(QStringLiteral("entry")).toString(), text(n.value(QStringLiteral("text"))),
                    at(n.value(QStringLiteral("day")).toInt(), n.value(QStringLiteral("time")).toString()), device);
    }

    for (const QJsonValue &v : world.value(QStringLiteral("rounds")).toArray()) {
        const QJsonObject o = v.toObject();
        PracticeLog::ExamRound round;
        round.at = at(o.value(QStringLiteral("day")).toInt(), o.value(QStringLiteral("time")).toString());
        round.discipline = o.value(QStringLiteral("discipline")).toString();
        round.grade = o.value(QStringLiteral("grade")).toString();
        round.count = o.value(QStringLiteral("count")).toInt();
        round.average = o.value(QStringLiteral("average")).toDouble();
        round.device = device;
        log.addRound(round, {});
    }
}

// The event of the course calendar a notebook was made from.
CourseBook::Course courseOf(const QJsonObject &event)
{
    CourseBook::Course c;
    c.calendarUid = event.value(QStringLiteral("uid")).toString();
    c.title = text(event.value(QStringLiteral("title")));
    c.first = dayOf(event.value(QStringLiteral("first")));
    c.last = dayOf(event.value(QStringLiteral("last")));
    c.place = text(event.value(QStringLiteral("place")));
    c.teacher = event.value(QStringLiteral("teacher")).toString();
    c.info = text(event.value(QStringLiteral("info")));
    return c;
}

// Grades, starts, course notebooks and the planned exam.
void seedCourses(const QJsonObject &world, const QString &device)
{
    CourseBook book;
    if (!book.open(Courses::defaultPath())) {
        qWarning("Demo: %s", qPrintable(book.errorString()));
        return;
    }
    const QDateTime now = QDateTime::currentDateTimeUtc();

    for (const QJsonValue &v : world.value(QStringLiteral("grades")).toArray()) {
        const QJsonObject o = v.toObject();
        CourseBook::Grade g;
        g.discipline = o.value(QStringLiteral("discipline")).toString();
        g.grade = o.value(QStringLiteral("grade")).toString();
        g.day = dayOf(o.value(QStringLiteral("day")));
        g.place = text(o.value(QStringLiteral("place")));
        book.addGrade(g, now, device);
    }
    for (const QJsonValue &v : world.value(QStringLiteral("starts")).toArray()) {
        const QJsonObject o = v.toObject();
        book.setStart(o.value(QStringLiteral("discipline")).toString(), dayOf(o.value(QStringLiteral("day"))), now, device);
    }

    // A notebook or a planned exam keeps a copy of its calendar event.
    QHash<QString, QJsonObject> events;
    for (const QJsonValue &v : world.value(QStringLiteral("calendar")).toArray()) {
        events.insert(v.toObject().value(QStringLiteral("uid")).toString(), v.toObject());
    }
    QHash<QString, QString> made;       // calendar uid -> notebook uid
    const auto notebook = [&](const QString &event) {
        if (!made.contains(event)) {
            made.insert(event, book.addCourse(courseOf(events.value(event)), now, device).uid);
        }
        return made.value(event);
    };

    for (const QJsonValue &v : world.value(QStringLiteral("notebooks")).toArray()) {
        const QJsonObject o = v.toObject();
        const QString course = notebook(o.value(QStringLiteral("course")).toString());
        for (const QJsonValue &e : o.value(QStringLiteral("entries")).toArray()) {
            const QJsonObject entry = e.toObject();
            const QDateTime when = at(entry.value(QStringLiteral("day")).toInt(), entry.value(QStringLiteral("time")).toString());
            const QString uid = book.addEntry(course, text(entry.value(QStringLiteral("text"))), when, device).uid;
            for (const QJsonValue &t : entry.value(QStringLiteral("techniques")).toArray()) {
                book.addTag(uid, t.toString(), when, device);
            }
        }
    }

    const QJsonObject exam = world.value(QStringLiteral("exam")).toObject();
    if (!exam.isEmpty()) {
        book.planExam(notebook(exam.value(QStringLiteral("course")).toString()), exam.value(QStringLiteral("discipline")).toString(),
                      exam.value(QStringLiteral("grade")).toString(), now);
    }
}

// ICS text: backslash, comma, semicolon and line breaks escaped.
QString icsText(QString s)
{
    return s.replace(QLatin1Char('\\'), QLatin1String("\\\\"))
        .replace(QLatin1Char(','), QLatin1String("\\,"))
        .replace(QLatin1Char(';'), QLatin1String("\\;"))
        .replace(QLatin1Char('\n'), QLatin1String("\\n"));
}

// The course calendar at MITORI_COURSES_CALENDAR (a file URL), dates around today. The teacher
// is the description's first line "mit …", as in the association's calendar.
void writeCalendar(const QJsonObject &world)
{
    const QUrl url(qEnvironmentVariable("MITORI_COURSES_CALENDAR"));
    if (!url.isLocalFile()) {
        return;
    }
    QStringList lines{QStringLiteral("BEGIN:VCALENDAR"), QStringLiteral("VERSION:2.0"), QStringLiteral("PRODID:-//Mitori demo//EN")};
    for (const QJsonValue &v : world.value(QStringLiteral("calendar")).toArray()) {
        const CourseBook::Course c = courseOf(v.toObject());
        lines << QStringLiteral("BEGIN:VEVENT") << QStringLiteral("UID:") + c.calendarUid
              << QStringLiteral("SUMMARY:") + icsText(c.title)
              << QStringLiteral("DTSTART;VALUE=DATE:") + c.first.toString(QStringLiteral("yyyyMMdd"))
              << QStringLiteral("DTEND;VALUE=DATE:") + c.last.addDays(1).toString(QStringLiteral("yyyyMMdd"))
              << QStringLiteral("LOCATION:") + icsText(c.place)
              << QStringLiteral("DESCRIPTION:") + icsText(QStringLiteral("mit ") + c.teacher + QStringLiteral("\n\n") + c.info)
              << QStringLiteral("END:VEVENT");
    }
    lines << QStringLiteral("END:VCALENDAR");

    QDir().mkpath(QFileInfo(url.toLocalFile()).absolutePath());
    QSaveFile file(url.toLocalFile());
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning("Demo: calendar not written: %s", qPrintable(file.errorString()));
        return;
    }
    file.write(lines.join(QLatin1String("\r\n")).toUtf8() + "\r\n");
    file.commit();
}

// One step of a shot: JavaScript in the window's scope (Main.qml: root, pageStack, the singletons).
void run(QObject *window, const QString &script)
{
    QQmlExpression expression(qmlContext(window), window, script);
    expression.evaluate();
    if (expression.hasError()) {
        qWarning("Demo: step failed: %s", qPrintable(expression.error().toString()));
    }
}

} // namespace

void Demo::prepare()
{
    writeSettings();
    const QJsonObject world = dojo();
    writeCalendar(world);

    // The data once per data folder (demo/start.sh runs the app once per style).
    if (QFileInfo::exists(Practice::defaultPath())) {
        return;
    }
    QDir().mkpath(QFileInfo(Practice::defaultPath()).absolutePath());
    const QString device = Practice::deviceId();
    seedPractice(world, device);
    seedCourses(world, device);
}

void Demo::shoot(QObject *object)
{
    auto *window = qobject_cast<QQuickWindow *>(object);
    const QJsonArray plan = QJsonDocument::fromJson(qgetenv("SHOT_PLAN")).array();
    const QDir dir(qEnvironmentVariable("SHOT_DIR"));
    if (!window || plan.isEmpty() || dir.path() == QLatin1String(".")) {
        return;
    }

    // Each shot: back to the start page, its size, its steps one after another, the picture.
    int delay = kSettleMs;
    for (const QJsonValue &v : plan) {
        const QJsonObject shot = v.toObject();
        const QJsonArray size = shot.value(QStringLiteral("viewport")).toArray();
        QTimer::singleShot(delay, window, [window, size] {
            window->resize(size.size() == 2 ? QSize(size[0].toInt(), size[1].toInt()) : QSize(kPhoneWidth, kPhoneHeight));
            run(window, QStringLiteral("pageStack.pop(pageStack.get(0))"));
        });
        delay += kSettleMs;

        for (const QJsonValue &step : shot.value(QStringLiteral("steps")).toArray()) {
            QTimer::singleShot(delay, window, [window, script = step.toString()] { run(window, script); });
            delay += kSettleMs;
        }
        delay += kSettleMs;

        QTimer::singleShot(delay, window, [window, dir, name = shot.value(QStringLiteral("name")).toString()] {
            if (!window->grabWindow().save(dir.filePath(name + QStringLiteral(".png")))) {
                qWarning("Demo: shot %s not saved", qPrintable(name));
            }
        });
        delay += kSettleMs / 3;
    }
    QTimer::singleShot(delay, window, &QCoreApplication::quit);
}
