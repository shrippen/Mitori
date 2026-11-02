#include <ctime>

#include <QDir>
#include <QFile>
#include <QQmlContext>
#include <QQmlEngine>
#include <QSettings>
#include <QtQml/QQmlExtensionPlugin>
#include <QtQuickTest>

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
        QFile::remove(Practice::defaultPath());
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
