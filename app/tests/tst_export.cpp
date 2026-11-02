#include <QBuffer>
#include <QFile>
#include <QImage>
#include <QProcess>
#include <QSettings>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTest>

#include "catalog.h"
#include "courses.h"
#include "mediastore.h"
#include "notebookexport.h"
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
        QImage image(120, 80, QImage::Format_RGB32);
        image.fill(Qt::gray);
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
        }
        QCOMPARE(courses.exportNotebook(uid, QUrl::fromLocalFile(zip)), 1);
        QCOMPARE(courses.errorString(), QString());
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
