#include <QDir>
#include <QGuiApplication>
#include <QIcon>
#include <QLocale>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QTranslator>
#include <QtQml/QQmlExtensionPlugin>

#include "appearance.h"

Q_IMPORT_QML_PLUGIN(MitoriPlugin)

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    QGuiApplication::setApplicationName(QStringLiteral("Mitori"));
    QGuiApplication::setOrganizationDomain(QStringLiteral("shrippen.github.io"));
    QGuiApplication::setDesktopFileName(QStringLiteral("io.github.shrippen.Mitori"));
    QGuiApplication::setApplicationVersion(QStringLiteral(MITORI_VERSION));
    // The bundled Breeze subset (scripts/sync-icons.sh): the theme where there is none
    // (Android, AppImage), else the fallback behind the system theme.
    QIcon::setThemeSearchPaths(QIcon::themeSearchPaths() << QStringLiteral(":/icons"));
    QIcon::setFallbackThemeName(QStringLiteral("breeze-mitori"));
    if (QIcon::themeName().isEmpty() || QIcon::themeName() == QLatin1String("hicolor")) {
        QIcon::setThemeName(QStringLiteral("breeze-mitori"));
    }

    // The language chosen in Appearance, else the first of the user's languages with a catalog
    // (de, en); otherwise the English of the code. Dates follow the chosen language too.
    const QString chosen = Appearance::startLocale();
    if (!chosen.isEmpty()) {
        QLocale::setDefault(QLocale(chosen));
    }
    QTranslator translator;
    if (translator.load(QLocale(), QStringLiteral("mitori"), QStringLiteral("_"), QStringLiteral(":/i18n"))) {
        QCoreApplication::installTranslator(&translator);
    }

#ifdef MITORI_DEMO
    // scripts/check-no-demo.sh looks for this mark in the binary: a release that carries it fails.
    // Split here, so shipped sources (Flatpak debug info) do not carry it.
    qInfo("MITORI-DEMO" "-BUILD");
#endif

    QQmlApplicationEngine engine;
    // The controls style of the chosen style (Appearance), before the first window.
    const Appearance::Style style = Appearance::startStyle(engine.importPathList());
    Appearance::prepare(style, &engine);
    if (qEnvironmentVariableIsEmpty("QT_QUICK_CONTROLS_STYLE")) {
        QQuickStyle::setStyle(Appearance::controlsStyle(style, engine.importPathList()));
    }
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed, &app,
                     [] { QCoreApplication::exit(1); }, Qt::QueuedConnection);
    engine.loadFromModule("Mitori", "Main");
    return app.exec();
}
