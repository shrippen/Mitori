#include "devicecalendar.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QTimeZone>
#include <QUrl>

#ifdef Q_OS_ANDROID
#include <QCoreApplication>
#include <QJniEnvironment>
#include <QJniObject>
#else
#include <QDesktopServices>
#endif

namespace {

// TEXT values of iCalendar: \ , ; and new lines escaped.
QString escaped(QString text)
{
    text.replace(QLatin1Char('\\'), QLatin1String("\\\\"));
    text.replace(QLatin1Char(','), QLatin1String("\\,"));
    text.replace(QLatin1Char(';'), QLatin1String("\\;"));
    text.replace(QLatin1Char('\n'), QLatin1String("\\n"));
    return text;
}

// Lines longer than 75 octets are folded (CRLF and a space), not inside a UTF-8 character.
QByteArray folded(const QString &line)
{
    const QByteArray utf8 = line.toUtf8();
    QByteArray out;
    int length = 0;
    for (qsizetype i = 0; i < utf8.size(); ++i) {
        const bool continuation = (uchar(utf8.at(i)) & 0xC0) == 0x80;
        if (length >= 74 && !continuation) {
            out += "\r\n ";
            length = 1;
        }
        out += utf8.at(i);
        ++length;
    }
    return out + "\r\n";
}

#ifdef Q_OS_ANDROID
// Whole-day events: from midnight UTC of the first day to midnight UTC after the last.
qint64 utcMidnight(const QDate &day)
{
    return QDateTime(day, QTime(0, 0), QTimeZone::UTC).toMSecsSinceEpoch();
}

bool addOnAndroid(const DeviceCalendar::Event &e)
{
    const QJniObject context = QNativeInterface::QAndroidApplication::context();
    if (!context.isValid()) {
        return false;
    }
    const QJniObject uri = QJniObject::callStaticObjectMethod(
        "android/net/Uri", "parse", "(Ljava/lang/String;)Landroid/net/Uri;",
        QJniObject::fromString(QStringLiteral("content://com.android.calendar/events")).object<jstring>());
    QJniObject intent("android/content/Intent", "(Ljava/lang/String;Landroid/net/Uri;)V",
                      QJniObject::fromString(QStringLiteral("android.intent.action.INSERT")).object<jstring>(),
                      uri.object());
    const auto put = [&intent](const char *key, const QString &value) {
        intent.callObjectMethod("putExtra", "(Ljava/lang/String;Ljava/lang/String;)Landroid/content/Intent;",
                                QJniObject::fromString(QString::fromLatin1(key)).object<jstring>(),
                                QJniObject::fromString(value).object<jstring>());
    };
    put("title", e.title);
    put("eventLocation", e.place);
    put("description", e.description);
    intent.callObjectMethod("putExtra", "(Ljava/lang/String;J)Landroid/content/Intent;",
                            QJniObject::fromString(QStringLiteral("beginTime")).object<jstring>(), jlong(utcMidnight(e.first)));
    intent.callObjectMethod("putExtra", "(Ljava/lang/String;J)Landroid/content/Intent;",
                            QJniObject::fromString(QStringLiteral("endTime")).object<jstring>(),
                            jlong(utcMidnight(e.last.addDays(1))));
    intent.callObjectMethod("putExtra", "(Ljava/lang/String;Z)Landroid/content/Intent;",
                            QJniObject::fromString(QStringLiteral("allDay")).object<jstring>(), jboolean(true));
    constexpr jint kNewTask = 0x10000000;   // Intent.FLAG_ACTIVITY_NEW_TASK: the context may not be an activity
    intent.callObjectMethod("addFlags", "(I)Landroid/content/Intent;", kNewTask);

    QJniEnvironment env;
    context.callMethod<void>("startActivity", "(Landroid/content/Intent;)V", intent.object());
    if (env.checkAndClearExceptions()) {
        return false;      // no calendar app (ActivityNotFoundException)
    }
    return true;
}
#endif

} // namespace

namespace DeviceCalendar {

QByteArray ics(const Event &e)
{
    const QString day = QStringLiteral("yyyyMMdd");
    const QDate last = e.last.isValid() && e.last >= e.first ? e.last : e.first;
    QByteArray out = "BEGIN:VCALENDAR\r\nVERSION:2.0\r\nPRODID:-//shrippen//Mitori//EN\r\nBEGIN:VEVENT\r\n";
    out += folded(QStringLiteral("UID:") + e.uid);
    out += folded(QStringLiteral("DTSTAMP:") + QDateTime::currentDateTimeUtc().toString(QStringLiteral("yyyyMMdd'T'HHmmss'Z'")));
    out += folded(QStringLiteral("DTSTART;VALUE=DATE:") + e.first.toString(day));
    out += folded(QStringLiteral("DTEND;VALUE=DATE:") + last.addDays(1).toString(day));    // exclusive
    out += folded(QStringLiteral("SUMMARY:") + escaped(e.title));
    if (!e.place.isEmpty()) {
        out += folded(QStringLiteral("LOCATION:") + escaped(e.place));
    }
    if (!e.description.isEmpty()) {
        out += folded(QStringLiteral("DESCRIPTION:") + escaped(e.description));
    }
    out += "END:VEVENT\r\nEND:VCALENDAR\r\n";
    return out;
}

bool add(const Event &e)
{
    if (e.title.isEmpty() || !e.first.isValid()) {
        return false;
    }
#ifdef Q_OS_ANDROID
    return addOnAndroid(e);
#else
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    QDir().mkpath(dir);
    QFile file(dir + QStringLiteral("/lehrgang.ics"));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate) || file.write(ics(e)) < 0) {
        return false;
    }
    file.close();
    return QDesktopServices::openUrl(QUrl::fromLocalFile(file.fileName()));
#endif
}

}
