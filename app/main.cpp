#include <QDir>
#include <QGuiApplication>
#include <QLocale>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QTranslator>

// Kirigami's own style (qqc2-desktop-style) where it is installed, as on Plasma; an AppImage
// or Android has only Material.
static QString controlsStyle(const QQmlApplicationEngine &engine)
{
    if (!qEnvironmentVariableIsSet("KDE_FULL_SESSION")) {
        return QStringLiteral("Material");
    }
    for (const QString &path : engine.importPathList()) {
        if (QDir(path).exists(QStringLiteral("org/kde/desktop"))) {
            return QStringLiteral("org.kde.desktop");
        }
    }
    return QStringLiteral("Material");
}

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    QGuiApplication::setApplicationName(QStringLiteral("Mitori"));
    QGuiApplication::setOrganizationDomain(QStringLiteral("shrippen.github.io"));
    QGuiApplication::setDesktopFileName(QStringLiteral("io.github.shrippen.Mitori"));

    // German from i18n/mitori_de.ts; any other language keeps the English of the code.
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
    if (qEnvironmentVariableIsEmpty("QT_QUICK_CONTROLS_STYLE")) {
        QQuickStyle::setStyle(controlsStyle(engine));
    }
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed, &app,
                     [] { QCoreApplication::exit(1); }, Qt::QueuedConnection);
    engine.loadFromModule("Mitori", "Main");
    return app.exec();
}
