#include <QFile>
#include <QFileInfo>
#include <QImage>
#include <QSettings>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTest>

#include "courses.h"
#include "mediastore.h"
#include "practice.h"

using namespace Qt::StringLiterals;

// Photos and voice notes: the media folder, and notes and entries that keep them.
class TstMedia : public QObject
{
    Q_OBJECT

    QTemporaryDir m_dir;

    QString picture(const QString &name, const QSize &size)
    {
        QImage image(size, QImage::Format_RGB32);
        image.fill(Qt::darkRed);
        const QString path = m_dir.filePath(name);
        image.save(path, "PNG");
        return path;
    }

private slots:
    void initTestCase()
    {
        QStandardPaths::setTestModeEnabled(true);
        QSettings().clear();
        QVERIFY(m_dir.isValid());
        QDir(MediaStore::defaultRoot()).removeRecursively();
    }

    // The storage bar's figure: all files of the folder, a missing folder counts nothing.
    void sizeOfTheFolder()
    {
        MediaStore store(m_dir.filePath(u"sized"_s));
        QCOMPARE(store.size(), 0);
        const QString name = store.importPhoto(QUrl::fromLocalFile(picture(u"sized.png"_s, {40, 30})));
        QVERIFY(!name.isEmpty());
        QCOMPARE(store.size(), QFileInfo(store.path(name)).size());
        QVERIFY(store.size() > 0);
    }

    // A large picture comes in smaller, as JPEG; something that is no picture does not.
    void importPhoto()
    {
        MediaStore store(m_dir.filePath(u"store"_s));
        const QString name = store.importPhoto(QUrl::fromLocalFile(picture(u"big.png"_s, {4000, 3000})));
        QVERIFY(!name.isEmpty());
        QCOMPARE(MediaStore::kindOf(name), MediaStore::Kind::Photo);
        const QImage in(store.path(name));
        QCOMPARE(in.size(), QSize(2048, 1536));
        QCOMPARE(QImage(store.path(store.importPhoto(QUrl::fromLocalFile(picture(u"small.png"_s, {300, 200}))))).size(),
                 QSize(300, 200));

        QFile text(m_dir.filePath(u"note.txt"_s));
        QVERIFY(text.open(QIODevice::WriteOnly));
        text.write("keine Bilddatei");
        text.close();
        QVERIFY(store.importPhoto(QUrl::fromLocalFile(text.fileName())).isEmpty());
        QVERIFY(store.importPhoto(QUrl::fromLocalFile(m_dir.filePath(u"missing.png"_s))).isEmpty());

        QVERIFY(store.remove(name));
        QVERIFY(!QFile::exists(store.path(name)));
        QVERIFY(!store.remove(name));
    }

    // Names from notes stay inside the folder.
    void namesStayInTheFolder()
    {
        MediaStore store(m_dir.filePath(u"store"_s));
        QVERIFY(store.path(u"../mitori.sqlite"_s).isEmpty());
        QVERIFY(store.path(u"/etc/passwd"_s).isEmpty());
        QVERIFY(store.path(u".hidden"_s).isEmpty());
        QVERIFY(!store.remove(u"../x.jpg"_s));
        const QString voice = store.newVoiceName();
        QCOMPARE(MediaStore::kindOf(voice), MediaStore::Kind::Voice);
        QVERIFY(store.path(voice).startsWith(store.root()));
    }

    // A note may be a photo only; deleting it deletes its file. Its text may go, not the photo.
    void noteWithPhoto()
    {
        Practice practice(m_dir.filePath(u"practice.sqlite"_s));
        const QString name = MediaStore::shared().importPhoto(QUrl::fromLocalFile(picture(u"foto.png"_s, {800, 600})));
        QVERIFY(!name.isEmpty());
        QVERIFY(!practice.addNote(u"e"_s, u" "_s));
        QVERIFY(practice.addNote(u"e"_s, {}, name));
        const QVariantMap note = practice.notes(u"e"_s).first().toMap();
        QCOMPARE(note.value(u"media"_s).toString(), name);
        QCOMPARE(note.value(u"text"_s).toString(), QString());
        QVERIFY(practice.changeNote(note.value(u"uid"_s).toString(), u"Hüfte, Skizze"_s));
        QVERIFY(practice.changeNote(note.value(u"uid"_s).toString(), u""_s));
        QVERIFY(practice.addNote(u"e"_s, u"nur Text"_s));
        const QString plain = practice.notes(u"e"_s).last().toMap().value(u"uid"_s).toString();
        QVERIFY(!practice.changeNote(plain, u" "_s));
        QVERIFY(QFile::exists(MediaStore::shared().path(name)));
        QVERIFY(practice.deleteNote(note.value(u"uid"_s).toString()));
        QVERIFY(!QFile::exists(MediaStore::shared().path(name)));
    }

    // A course entry the same; deleting the notebook deletes the files of its entries.
    void entryWithVoice()
    {
        Catalog catalog;
        Courses courses(m_dir.filePath(u"courses.sqlite"_s), QUrl::fromLocalFile(m_dir.filePath(u"none.ics"_s)),
                        m_dir.filePath(u"cache.ics"_s), catalog);
        const QString course = courses.create(u"Lehrgang"_s, u"2026-07-01"_s, {}, {}, {});
        const QString voice = MediaStore::shared().newVoiceName();
        QFile f(MediaStore::shared().path(voice));
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write("aac");
        f.close();
        QVERIFY(courses.addEntry(course, {}, voice));
        QCOMPARE(courses.entries(course).first().toMap().value(u"media"_s).toString(), voice);
        const QString photo = MediaStore::shared().importPhoto(QUrl::fromLocalFile(picture(u"tafel.png"_s, {400, 300})));
        QVERIFY(courses.addEntry(course, u"Tafel"_s, photo));
        const QString uid = courses.entries(course).last().toMap().value(u"uid"_s).toString();
        QVERIFY(courses.deleteEntry(uid));
        QVERIFY(!QFile::exists(MediaStore::shared().path(photo)));
        QVERIFY(QFile::exists(MediaStore::shared().path(voice)));
        QVERIFY(courses.remove(course));
        QVERIFY(!QFile::exists(MediaStore::shared().path(voice)));
    }
};

QTEST_GUILESS_MAIN(TstMedia)
#include "tst_media.moc"
