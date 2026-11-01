#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickStyle>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    QGuiApplication::setApplicationName(QStringLiteral("Mitori"));
    QGuiApplication::setOrganizationDomain(QStringLiteral("shrippen.github.io"));
    QGuiApplication::setDesktopFileName(QStringLiteral("io.github.shrippen.Mitori"));
    // Kirigami's own style on Plasma, Material elsewhere (Android).
    if (qEnvironmentVariableIsEmpty("QT_QUICK_CONTROLS_STYLE")) {
        QQuickStyle::setStyle(qEnvironmentVariableIsSet("KDE_FULL_SESSION")
                                  ? QStringLiteral("org.kde.desktop") : QStringLiteral("Material"));
    }

#ifdef MITORI_DEMO
    // scripts/check-no-demo.sh looks for this mark: a release that carries it fails.
    qInfo("MITORI-DEMO-BUILD");
#endif

    QQmlApplicationEngine engine;
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed, &app,
                     [] { QCoreApplication::exit(1); }, Qt::QueuedConnection);
    engine.loadFromModule("Mitori", "Main");
    return app.exec();
}
