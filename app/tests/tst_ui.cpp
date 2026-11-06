#include <ctime>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrl>
#include <QVideoFrame>
#include <QVideoSink>
#include <QQmlContext>
#include <QQmlEngine>
#include <QSettings>
#include <QStandardPaths>
#include <QTranslator>
#include <QUrl>
#include <QtQml/QQmlExtensionPlugin>
#include <QtQuickTest>

#include "appearance.h"
#include "courses.h"
#include "bundleshelf.h"
#include "catalog.h"
#include "mediastore.h"
#include "practice.h"
#include "qrimage.h"

Q_IMPORT_QML_PLUGIN(MitoriPlugin)

class Setup : public QObject
{
    Q_OBJECT

public:
    // A text from the German catalog ("" without one): how long a page's text is in German, while
    // the run stays English (nothing is installed).
    Q_INVOKABLE QString german(const QString &context, const QString &source)
    {
        if (m_german.isEmpty()) {
            (void)m_german.load(QStringLiteral(":/i18n/mitori_de"));
        }
        return m_german.translate(context.toUtf8().constData(), source.toUtf8().constData());
    }

    // The fake camera (services/codescanner): what stands in for the device, and a QR code or an
    // empty picture ("") as a frame of the page's sink.
    Q_INVOKABLE void setCamera(const QString &stand) { qputenv("MITORI_CAMERA", stand.toUtf8()); }
    Q_INVOKABLE void showFrame(QVideoSink *sink, const QString &text)
    {
        QImage image(360, 360, QImage::Format_Grayscale8);
        image.fill(128);
        sink->setVideoFrame(QVideoFrame(text.isEmpty() ? image : qrImage(text)));
    }

public slots:
    void applicationAvailable()
    {
        QCoreApplication::setApplicationName(QStringLiteral("mitori-ui-test"));
        QCoreApplication::setOrganizationDomain(QStringLiteral("shrippen.github.io"));   // as main.cpp (Settings)
        // West of UTC: a QDate reaches QML as midnight UTC, the day before here (catches "today"
        // compared that way; NotesPage once did).
        qputenv("TZ", "America/Los_Angeles");
        tzset();
        // Data and settings in the build directory, so that runs in different builds (worktrees,
        // reviews) do not share ~/.qttest and delete each other's database.
        qputenv("XDG_DATA_HOME", UI_DATA_DIR "/data");
        qputenv("XDG_CONFIG_HOME", UI_DATA_DIR "/config");
        qputenv("XDG_CACHE_HOME", UI_DATA_DIR "/cache");
        // Every run starts from the defaults (the video folder from MITORI_VIDEOS, not from the
        // setting a test wrote last time).
        QSettings().clear();
        // The first-start wizard has its own case (test_0_welcome); the others start on the menu.
        QSettings().setValue(QStringLiteral("look/welcomed"), true);
        // The shared grade of an older version (test_0a_practice_migrated).
        QSettings().setValue(QStringLiteral("programme/grade/shinkitoho"), QStringLiteral("chuden"));
        // A speech engine without a Japanese voice, the same on every machine (the glossary
        // hides its speaker buttons).
        qputenv("MITORI_SPEECH_ENGINE", "mock");
        QFile::remove(Practice::defaultPath());
        QFile::remove(Courses::defaultPath());
        QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)).remove(QStringLiteral("lehrgaenge.ics"));
        // The course calendar from a file, dates relative to today: never the network.
        const auto day = [](int offset) { return QDate::currentDate().addDays(offset).toString(QStringLiteral("yyyyMMdd")).toUtf8(); };
        QDir().mkpath(QStringLiteral(UI_DATA_DIR));
        QFile calendar(QStringLiteral(UI_DATA_DIR "/lehrgaenge.ics"));
        if (calendar.open(QIODevice::WriteOnly)) {
            calendar.write("BEGIN:VCALENDAR\r\nVERSION:2.0\r\n"
                           "BEGIN:VEVENT\r\nUID:ui-daito\r\nSUMMARY:Daitoryu Aiki Jujutsu\r\nDTSTART;VALUE=DATE:" + day(12)
                           + "\r\nDTEND;VALUE=DATE:" + day(14) + "\r\nLOCATION:Siebeldingen\r\n"
                           "DESCRIPTION:mit M. D. Nakajima Shihan\\nSamstag\\nAusschreibung: https://example.org/a?b=1\r\nEND:VEVENT\r\n"
                           "BEGIN:VEVENT\r\nUID:ui-aikido\r\nSUMMARY:Aikido-Prüfungsvorbereitung\r\nDTSTART;VALUE=DATE:" + day(40)
                           + "\r\nDTEND;VALUE=DATE:" + day(42) + "\r\nLOCATION:Pforzheim\r\nEND:VEVENT\r\n"
                           "END:VCALENDAR\r\n");
        }
        // Brightness and volume in memory: no test dims the screen (platform/devicelevels).
        qputenv("MITORI_DEVICE_LEVELS", "memory");
        // No test turns a camera on; frames come from showFrame.
        qputenv("MITORI_CAMERA", "frames");
        qputenv("MITORI_COURSES_CALENDAR", QUrl::fromLocalFile(calendar.fileName()).toString().toUtf8());
        // Video bundles from the test data (tools/bundle.py), as a local package folder: never the network.
        QDir(BundleShelf::defaultRoot()).removeRecursively();
        // A voice note on the device for the notes' player (a recorder needs a microphone).
        QDir(MediaStore::defaultRoot()).removeRecursively();
        QDir().mkpath(MediaStore::defaultRoot());
        QFile::copy(QStringLiteral(TEST_MEDIA_DIR "/stimme.m4a"), MediaStore::shared().path(QStringLiteral("test-stimme.m4a")));
        // Two folders for the player, each with a file it really plays (sound only, no picture).
        for (const QString &name : {QStringLiteral("a"), QStringLiteral("b")}) {
            const QString played = QStringLiteral(UI_DATA_DIR "/played-") + name + QStringLiteral("/Aikido/5. Kyu.mkv");
            QDir().mkpath(QFileInfo(played).absolutePath());
            QFile::remove(played);
            QFile::copy(QStringLiteral(TEST_MEDIA_DIR "/stimme.m4a"), played);
        }
        const QString packages = QStringLiteral(UI_DATA_DIR "/packages");
        const QString video = packages + QStringLiteral("/test-bundle/1/test.mvb");
        QDir().mkpath(QFileInfo(video).absolutePath());
        QFile::remove(video);
        QFile::copy(QStringLiteral(TEST_BUNDLE_DIR "/test.mvb"), video);
        QFile source(QStringLiteral(TEST_BUNDLE_DIR "/manifest.json"));
        QFile manifest(QStringLiteral(UI_DATA_DIR "/manifest.json"));
        if (source.open(QIODevice::ReadOnly) && manifest.open(QIODevice::WriteOnly)) {
            manifest.write(source.readAll().replace("file:///unused/test.mvb", QUrl::fromLocalFile(video).toEncoded()));
        }
        qputenv("MITORI_BUNDLES_MANIFEST", QUrl::fromLocalFile(manifest.fileName()).toString().toUtf8());
    }

    void qmlEngineAvailable(QQmlEngine *engine)
    {
        // The app's bundled icons, as on Android (main.cpp).
        QIcon::setThemeSearchPaths({QStringLiteral(":/icons")});
        QIcon::setThemeName(QStringLiteral("breeze-mitori"));
        const bool shinki = qEnvironmentVariable("MITORI_TEST_STYLE") == QLatin1String("shinki");
        const QString shots = QStringLiteral(UI_SHOTS_DIR) + (shinki ? QStringLiteral("/shinki") : QString());
        QDir().mkpath(shots);
        engine->rootContext()->setContextProperty(QStringLiteral("uiDataDir"), QStringLiteral(UI_DATA_DIR));
        engine->rootContext()->setContextProperty(QStringLiteral("uiShotsDir"), shots);
        engine->rootContext()->setContextProperty(QStringLiteral("uiShinki"), shinki);
        engine->rootContext()->setContextProperty(QStringLiteral("uiCatalog"), this);
        // The second run (ctest ui_shinki): the pages through Look as Shinki. The running style
        // belongs to it too: the window's head and the global tool bar follow it (as after a
        // start with Shinki).
        if (shinki) {
            QSettings().setValue(QStringLiteral("look/running"), int(Appearance::Style::Shinki));
            Appearance::prepare(Appearance::Style::Shinki, engine);
        }
        engine->rootContext()->setContextProperty(QStringLiteral("testMediaDir"), QStringLiteral(TEST_MEDIA_DIR));
        // Empty files under the catalog's names: pages take them for videos, nothing plays. The
        // tablet's page can be tested without the videos (the CI has none).
        const QString stand = QStringLiteral(UI_DATA_DIR "/stand-in-videos");
        const QVariantMap videos = Catalog::bundled().videos();
        for (const QVariant &video : videos) {
            const QString file = stand + QLatin1Char('/') + video.toMap().value(QStringLiteral("datei")).toString();
            QDir().mkpath(QFileInfo(file).path());
            QFile standIn(file);
            if (!standIn.open(QIODevice::WriteOnly)) {
                qFatal("no stand-in video: %s", qPrintable(file));
            }
        }
        engine->rootContext()->setContextProperty(QStringLiteral("standInVideos"), stand);
    }

private:
    QTranslator m_german;
};

QUICK_TEST_MAIN_WITH_SETUP(ui, Setup)
#include "tst_ui.moc"
