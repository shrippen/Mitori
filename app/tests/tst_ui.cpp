#include <ctime>

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrl>
#include <QQmlContext>
#include <QQmlEngine>
#include <QSettings>
#include <QStandardPaths>
#include <QUrl>
#include <QtQml/QQmlExtensionPlugin>
#include <QtQuickTest>

#include "courses.h"
#include "bundleshelf.h"
#include "practice.h"

Q_IMPORT_QML_PLUGIN(MitoriPlugin)

class Setup : public QObject
{
    Q_OBJECT

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
        qputenv("MITORI_COURSES_CALENDAR", QUrl::fromLocalFile(calendar.fileName()).toString().toUtf8());
        // Video bundles from the test data (tools/bundle.py), as a local package folder: never the network.
        QDir(BundleShelf::defaultRoot()).removeRecursively();
        const QString packages = QStringLiteral(UI_DATA_DIR "/packages");
        QDir().mkpath(packages + QStringLiteral("/mitori-test-bundle/1"));
        QFile::remove(packages + QStringLiteral("/mitori-test-bundle/1/test.mvb"));
        QFile::copy(QStringLiteral(TEST_BUNDLE_DIR "/test.mvb"), packages + QStringLiteral("/mitori-test-bundle/1/test.mvb"));
        QFile source(QStringLiteral(TEST_BUNDLE_DIR "/manifest.json"));
        QFile manifest(QStringLiteral(UI_DATA_DIR "/manifest.json"));
        if (source.open(QIODevice::ReadOnly) && manifest.open(QIODevice::WriteOnly)) {
            QJsonObject m = QJsonDocument::fromJson(source.readAll()).object();
            m.insert(QStringLiteral("basis"), QUrl::fromLocalFile(packages).toString());
            manifest.write(QJsonDocument(m).toJson());
        }
        qputenv("MITORI_BUNDLES_MANIFEST", QUrl::fromLocalFile(manifest.fileName()).toString().toUtf8());
    }

    void qmlEngineAvailable(QQmlEngine *engine)
    {
        // The app's bundled icons, as on Android (main.cpp).
        QIcon::setThemeSearchPaths({QStringLiteral(":/icons")});
        QIcon::setThemeName(QStringLiteral("breeze-mitori"));
        QDir().mkpath(QStringLiteral(UI_SHOTS_DIR));
        engine->rootContext()->setContextProperty(QStringLiteral("uiShotsDir"), QStringLiteral(UI_SHOTS_DIR));
    }
};

QUICK_TEST_MAIN_WITH_SETUP(ui, Setup)
#include "tst_ui.moc"
