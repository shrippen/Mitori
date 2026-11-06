#include <QBuffer>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QProcess>
#include <QScopeGuard>
#include <QSettings>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTest>

#include "catalog.h"
#include "coursebook.h"
#include "courses.h"
#include "mediastore.h"
#include "notebookexport.h"
#include "saveplace.h"
#include "sharefile.h"
#include "zipwriter.h"

using namespace Qt::StringLiterals;

// The notebook export: the ZIP writer, the Markdown, and the whole archive read back by
// Python's zipfile (an independent reader).
class TstExport : public QObject
{
    Q_OBJECT

    QTemporaryDir m_dir;

    // "name<TAB>size" per file and the text of notizbuch.md, from Python; empty without python3.
    static QString readBack(const QString &zip)
    {
        QProcess p;
        p.start(u"python3"_s, {u"-I"_s, u"-c"_s,
                               u"import sys, zipfile\n"
                               "z = zipfile.ZipFile(sys.argv[1])\n"
                               "assert z.testzip() is None\n"
                               "for i in z.infolist(): print(i.filename + '\\t' + str(i.file_size))\n"
                               "print('---')\n"
                               "print(z.read('notizbuch.md').decode('utf-8'))"_s, zip});
        if (!p.waitForFinished(10000) || p.exitCode() != 0) {
            return u"failed: "_s + QString::fromUtf8(p.readAllStandardError());
        }
        return QString::fromUtf8(p.readAllStandardOutput());
    }

private slots:
    void initTestCase()
    {
        QStandardPaths::setTestModeEnabled(true);
        QSettings().clear();
        QVERIFY(m_dir.isValid());
        QDir(MediaStore::defaultRoot()).removeRecursively();
    }

    void crc32()
    {
        QCOMPARE(ZipWriter::crc32("123456789"), 0xCBF43926u);       // the standard check value
        QCOMPARE(ZipWriter::crc32({}), 0u);
    }

    // A notebook with text, a photo and a voice note: notizbuch.md by day, the files under
    // bilder/ and audio/, readable by another ZIP reader; a lost file is left out.
    void notebookArchive()
    {
        if (QStandardPaths::findExecutable(u"python3"_s).isEmpty()) {
            QSKIP("python3 reads the archive back");
        }
        Catalog catalog;
        Courses courses(m_dir.filePath(u"c.sqlite"_s), QUrl::fromLocalFile(m_dir.filePath(u"none.ics"_s)),
                        m_dir.filePath(u"cache.ics"_s), catalog);
        const QString uid = courses.create(u"Sommer: Lehrgang/Mainz"_s, u"2026-07-04"_s, u"2026-07-05"_s, u"Mainz"_s, u"Gast"_s);
        QVERIFY(courses.addEntry(uid, u"Tenkan: Fuß zuerst"_s));
        // Noise: a JPEG of some 100 KB, past QFile's 16 KB buffer (a write that fails at once).
        QImage image(400, 300, QImage::Format_RGB32);
        for (int y = 0; y < image.height(); ++y) {
            for (int x = 0; x < image.width(); ++x) {
                image.setPixel(x, y, qRgb((x * 37 + y * 91) % 256, (x * y) % 256, (x ^ y) % 256));
            }
        }
        image.save(m_dir.filePath(u"p.png"_s));
        const QString photo = MediaStore::shared().importPhoto(QUrl::fromLocalFile(m_dir.filePath(u"p.png"_s)));
        QVERIFY(courses.addEntry(uid, u"Tafel"_s, photo));
        const QString voice = MediaStore::shared().newVoiceName();
        QFile v(MediaStore::shared().path(voice));
        QVERIFY(v.open(QIODevice::WriteOnly));
        v.write(QByteArray(5000, 'a'));
        v.close();
        QVERIFY(courses.addEntry(uid, {}, voice));
        const QString lost = MediaStore::shared().newVoiceName();          // never written
        QVERIFY(courses.addEntry(uid, u"verloren"_s, lost));

        QCOMPARE(courses.exportName(uid), u"Sommer Lehrgang Mainz 2026-07-04.zip"_s);
        const QString zip = m_dir.filePath(courses.exportName(uid));
        // A failed export, then a good one: the error goes again; the lost file is counted.
        QCOMPARE(courses.exportNotebook(uid, QUrl::fromLocalFile(m_dir.filePath(u"no/such/dir/x.zip"_s))), -1);
        QVERIFY(!courses.errorString().isEmpty());
        // A full disk fails when the last pieces are flushed, not when they are written.
        if (QFile::exists(u"/dev/full"_s)) {
            QCOMPARE(courses.exportNotebook(uid, QUrl::fromLocalFile(u"/dev/full"_s)), -1);
            QVERIFY(!courses.errorString().isEmpty());
            // Safeguard: the message names the write error, not "Unknown error".
            QVERIFY2(!courses.errorString().contains(u"Unknown error"_s), qPrintable(courses.errorString()));
        }
        // An existing file the export cannot open stays (it is not ours to remove).
        const QString existing = m_dir.filePath(u"alt.zip"_s);
        QFile keep(existing);
        QVERIFY(keep.open(QIODevice::WriteOnly));
        keep.write("alte Sicherung");
        keep.close();
        QVERIFY(keep.setPermissions(QFileDevice::ReadOwner));

        // Root (the CI container) writes it anyway; the case needs a user without the right.
        QFile probe(existing);
        if (!probe.open(QIODevice::WriteOnly | QIODevice::Append)) {
            QCOMPARE(courses.exportNotebook(uid, QUrl::fromLocalFile(existing)), -1);
            QVERIFY(QFile::exists(existing));
        }
        probe.close();
        keep.setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner);
        QCOMPARE(courses.exportNotebook(uid, QUrl::fromLocalFile(zip)), 1);
        QCOMPARE(courses.errorString(), QString());

        // The sheet's preview says what the file holds, to the byte.
        const QVariantMap preview = courses.exportPreview(uid);
        QCOMPARE(preview.value(u"name"_s).toString(), courses.exportName(uid));
        QCOMPARE(preview.value(u"photos"_s).toInt(), 1);
        QCOMPARE(preview.value(u"voices"_s).toInt(), 1);
        QCOMPARE(preview.value(u"missing"_s).toInt(), 1);
        QCOMPARE(preview.value(u"bytes"_s).toLongLong(), QFileInfo(zip).size());
        QVERIFY(!preview.value(u"size"_s).toString().isEmpty());
        QVERIFY(preview.value(u"text"_s).toString().contains(u"](bilder/02-foto.jpg)"_s));
        QVERIFY(courses.exportPreview(u"no such notebook"_s).isEmpty());

        // The copy to share: one file, the last one, the same archive; the desktop has no dialog.
        int lostFiles = -1;
        const QString first = courses.shareCopy(uid, &lostFiles);
        QCOMPARE(lostFiles, 1);
        QCOMPARE(QFileInfo(first).fileName(), courses.exportName(uid));
        QVERIFY(first.startsWith(ShareFile::folder() + u'/'));
        QCOMPARE(QFileInfo(first).size(), QFileInfo(zip).size());
        const QString other = courses.create(u"Zweiter"_s, u"2026-08-01"_s, {}, {}, {});
        const QString second = courses.shareCopy(other);
        QVERIFY(!QFile::exists(first));
        QVERIFY(QFile::exists(second));

        // Deleted, its copy goes with it: text, photos and voice notes it held.
        QVERIFY(courses.remove(other));
        QVERIFY2(!QFile::exists(second), qPrintable(second));

        // Another notebook of the same file name: its deletion leaves this copy alone.
        const QString twinA = courses.create(u"Sommer: Mainz"_s, u"2026-07-04"_s, {}, {}, {});
        const QString twinB = courses.create(u"Sommer/Mainz"_s, u"2026-07-04"_s, {}, {}, {});
        QCOMPARE(courses.exportName(twinA), courses.exportName(twinB));
        const QString twinCopy = courses.shareCopy(twinA);
        QVERIFY(courses.remove(twinB));
        QVERIFY2(QFile::exists(twinCopy), qPrintable(twinCopy));

        // Deleted elsewhere: a sync takes it out of the database, then the notebooks are shown anew.
        {
            CourseBook book;
            QVERIFY(book.open(m_dir.filePath(u"c.sqlite"_s)));
            QVERIFY(book.deleteCourse(twinA, QDateTime::currentDateTimeUtc()));
        }
        courses.refresh();
        QVERIFY2(!QFile::exists(twinCopy), qPrintable(twinCopy));
        QVERIFY(courses.shareCopy(u"no such notebook"_s).isEmpty());
        QVERIFY(!courses.canShare());
        QCOMPARE(courses.shareNotebook(uid), -1);
        QVERIFY(!courses.errorString().isEmpty());
        QVERIFY(QDir(ShareFile::folder()).removeRecursively());

        const QString back = readBack(zip);
        QVERIFY2(!back.startsWith(u"failed"_s), qPrintable(back));
        const QStringList parts = back.split(u"---\n"_s);
        QStringList files = parts.first().trimmed().split(u'\n');
        QCOMPARE(files.size(), 3);
        QVERIFY(files.at(0).startsWith(u"bilder/02-foto.jpg\t"_s));
        QCOMPARE(files.at(1), u"audio/03-sprachnotiz.m4a\t5000"_s);
        QVERIFY(files.at(2).startsWith(u"notizbuch.md\t"_s));
        const QString md = parts.last();
        QVERIFY(md.startsWith(u"# Sommer: Lehrgang/Mainz\n"_s));
        QVERIFY(md.contains(u"· Mainz · Gast"_s));
        QVERIFY(md.contains(u"\n## "_s));
        QVERIFY(md.contains(u"Tenkan: Fuß zuerst"_s));
        QVERIFY(md.contains(u"](bilder/02-foto.jpg)"_s));
        QVERIFY(md.contains(u"](audio/03-sprachnotiz.m4a)"_s));
        QVERIFY(md.contains(u"verloren"_s));
        QVERIFY(!md.contains(u"04-"_s));                // the lost file is not linked
        QVERIFY(md.contains(u"verloren  \n(Voice note not on the device)"_s));
    }

    // The question before replacing: only a file, and the one the export would write. A folder
    // ("~/") is no file to replace; a name without its suffix means the name with it.
    void saveTarget()
    {
        SavePlace place;
        if (!place.asksItself()) {
            QSKIP("the portal's dialog asks in the Flatpak");
        }
        QVERIFY(QDir(m_dir.path()).mkdir(u"ordner"_s));
        const QUrl folder = QUrl::fromLocalFile(m_dir.path());
        QVERIFY(!place.taken(QUrl::fromLocalFile(m_dir.filePath(u"ordner"_s))));
        QVERIFY(!place.taken(QUrl::fromLocalFile(m_dir.filePath(u"ordner/"_s))));

        QFile old(m_dir.filePath(u"sicherung.alt.zip"_s));
        QVERIFY(old.open(QIODevice::WriteOnly));
        old.close();
        const QUrl typed = QUrl::fromLocalFile(m_dir.filePath(u"sicherung.alt"_s));
        QCOMPARE(place.resolve(typed, folder), QUrl::fromLocalFile(old.fileName()));
        QVERIFY(place.taken(place.resolve(typed, folder)));
    }

    // Round 2: in the Flatpak the portal hands out the granted file. Another name would be written
    // as a hidden file while the app reports success, so the path stays as it came.
    void portalPathStays()
    {
        const QByteArray before = qgetenv("FLATPAK_ID");
        qputenv("FLATPAK_ID", "io.github.shrippen.Mitori");
        const auto restore = qScopeGuard([&] {
            if (before.isEmpty()) {
                qunsetenv("FLATPAK_ID");
            } else {
                qputenv("FLATPAK_ID", before);
            }
        });

        SavePlace place;
        QVERIFY(!place.asksItself());
        const QUrl folder = QUrl::fromLocalFile(u"/run/user/1000/doc/ab12"_s);
        const QUrl granted = QUrl::fromLocalFile(u"/run/user/1000/doc/ab12/y.alt"_s);
        QCOMPARE(place.resolve(granted, folder), granted);

        // The export writes that very file.
        Catalog catalog;
        Courses courses(m_dir.filePath(u"portal.sqlite"_s), QUrl::fromLocalFile(m_dir.filePath(u"none.ics"_s)),
                        m_dir.filePath(u"cache-portal.ics"_s), catalog);
        const QString uid = courses.create(u"Portal"_s, u"2026-07-04"_s, {}, {}, {});
        const QUrl here = QUrl::fromLocalFile(m_dir.filePath(u"y.alt"_s));
        const QUrl target = place.resolve(here, QUrl::fromLocalFile(m_dir.path()));
        QCOMPARE(courses.exportNotebook(uid, target), 0);
        QVERIFY(QFile::exists(here.toLocalFile()));
        QVERIFY(!QFile::exists(here.toLocalFile() + u".zip"_s));
    }

    // Characters a URL or a file system takes for something else stay out of the name.
    void safeFileName()
    {
        CourseBook::Course c;
        c.title = u"Lehrgang #3: 100% Irimi?"_s;
        c.first = QDate(2026, 6, 6);
        QCOMPARE(NotebookExport::fileName(c), u"Lehrgang 3 100 Irimi 2026-06-06.zip"_s);
    }
};

QTEST_GUILESS_MAIN(TstExport)
#include "tst_export.moc"
