#include "notebookexport.h"

#include "mediastore.h"
#include "zipwriter.h"

#include <QCoreApplication>
#include <QFile>
#include <QLocale>
#include <QRegularExpression>

namespace {

QString tr(const char *text)
{
    return QCoreApplication::translate("NotebookExport", text);
}

// "6.6.2026 – 7.6.2026", or one day.
QString days(const CourseBook::Course &c)
{
    const QLocale locale;
    const QString first = locale.toString(c.first, QLocale::ShortFormat);
    return c.last.isValid() && c.last != c.first ? first + QStringLiteral(" – ") + locale.toString(c.last, QLocale::ShortFormat) : first;
}

// The archive's name of an entry's file: "bilder/01-foto.jpg", "audio/02-sprachnotiz.m4a".
QString archiveName(int number, const QString &media)
{
    const QString nr = QString::number(number).rightJustified(2, QLatin1Char('0'));
    return MediaStore::kindOf(media) == MediaStore::Kind::Photo ? QStringLiteral("bilder/%1-foto.jpg").arg(nr)
                                                                : QStringLiteral("audio/%1-sprachnotiz.m4a").arg(nr);
}

} // namespace

namespace NotebookExport {

QByteArray markdown(const CourseBook::Course &c, const QList<CourseBook::Entry> &entries, const QHash<QString, QString> &files)
{
    const QLocale locale;
    QStringList head{days(c), c.place, c.teacher};
    head.removeAll(QString());
    QString md = QStringLiteral("# %1\n\n%2\n").arg(c.title, head.join(QStringLiteral(" · ")));
    if (!c.announcement.isEmpty()) {
        md += QStringLiteral("\n%1: <%2>\n").arg(tr("Announcement"), c.announcement);
    }

    QDate day;
    for (const CourseBook::Entry &e : entries) {
        const QDateTime local = e.created.toLocalTime();
        if (local.date() != day) {
            day = local.date();
            md += QStringLiteral("\n## %1\n").arg(locale.toString(day, QLocale::LongFormat));
        }
        md += QStringLiteral("\n**%1** %2").arg(locale.toString(local.time(), QLocale::ShortFormat), e.text);
        const QString file = files.value(e.media);
        const bool photo = MediaStore::kindOf(e.media) == MediaStore::Kind::Photo;
        if (!e.media.isEmpty() && file.isEmpty()) {
            md += QStringLiteral("  \n(%1)").arg(photo ? tr("Photo not on the device") : tr("Voice note not on the device"));
        }
        if (!file.isEmpty()) {
            md += MediaStore::kindOf(e.media) == MediaStore::Kind::Photo
                      ? QStringLiteral("  \n![%1](%2)").arg(tr("Photo"), file)
                      : QStringLiteral("  \n[%1](%2)").arg(tr("Voice note"), file);
        }
        md += QLatin1Char('\n');
    }
    return md.toUtf8();
}

bool write(QIODevice *out, const CourseBook::Course &course, const QList<CourseBook::Entry> &entries,
           const MediaStore &media, int *missing)
{
    ZipWriter zip(out);
    QHash<QString, QString> files;
    int number = 0;
    int lost = 0;
    for (const CourseBook::Entry &e : entries) {
        ++number;
        if (e.media.isEmpty()) {
            continue;
        }
        QFile f(media.path(e.media));
        if (!f.open(QIODevice::ReadOnly)) {
            ++lost;
            continue;
        }
        const QString name = archiveName(number, e.media);
        if (!zip.add(name, f.readAll(), e.created)) {
            return false;
        }
        files.insert(e.media, name);
    }
    if (missing) {
        *missing = lost;
    }
    return zip.add(QStringLiteral("notizbuch.md"), markdown(course, entries, files)) && zip.finish();
}

QString fileName(const CourseBook::Course &c)
{
    // Also "#" and "%": a URL would take them for a fragment or an escape.
    static const QRegularExpression unsafe(QStringLiteral("[\\\\/:*?\"<>|#%\\x00-\\x1f]+"));
    QString title = c.title;
    title.replace(unsafe, QStringLiteral(" "));
    return title.simplified() + QLatin1Char(' ') + c.first.toString(Qt::ISODate) + QStringLiteral(".zip");
}

}
