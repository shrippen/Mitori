#include "appearance.h"

#include <QCoreApplication>
#include <QDir>
#include <QLocale>
#include <QDirIterator>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQmlFileSelector>
#include <QSettings>
#include <QTranslator>

#include "systembars.h"

namespace {
constexpr auto kStyleKey = "look/style";
constexpr auto kRunningKey = "look/running";     // what main.cpp started with (for the pages)
constexpr auto kLanguageKey = "look/language";
constexpr auto kWelcomedKey = "look/welcomed";
constexpr Appearance::Style kDefaultStyle = Appearance::Style::Leinen;

// Kirigami's own style (qqc2-desktop-style): installed and in a Plasma session.
bool hasBreeze(const QStringList &importPaths)
{
    if (!qEnvironmentVariableIsSet("KDE_FULL_SESSION")) {
        return false;
    }
    for (const QString &path : importPaths) {
        if (QDir(path).exists(QStringLiteral("org/kde/desktop"))) {
            return true;
        }
    }
    return false;
}

Appearance::Style saved()
{
    const int value = QSettings().value(QLatin1String(kStyleKey), int(kDefaultStyle)).toInt();
    if (value < int(Appearance::Style::Breeze) || value > int(Appearance::Style::Shinki)) {
        return kDefaultStyle;
    }
    return Appearance::Style(value);
}

Appearance::Language savedLanguage()
{
    return Appearance::Language(QSettings().value(QLatin1String(kLanguageKey), int(Appearance::Language::System)).toInt());
}

// The catalog of a language: the first of the user's languages with one (de, en) for System.
bool loadCatalog(QTranslator &catalog, Appearance::Language language)
{
    const QString name = Appearance::localeName(language);
    const QLocale locale = name.isEmpty() ? QLocale::system() : QLocale(name);
    return catalog.load(locale, QStringLiteral("mitori"), QStringLiteral("_"), QStringLiteral(":/i18n"));
}

// The one catalog the app runs with; the locale, so that dates and numbers follow.
void applyLanguage(Appearance::Language language)
{
    static QTranslator *running = nullptr;
    if (running) {
        QCoreApplication::removeTranslator(running);
        delete running;
    }
    running = new QTranslator(QCoreApplication::instance());
    const QString name = Appearance::localeName(language);
    QLocale::setDefault(name.isEmpty() ? QLocale::system() : QLocale(name));
    if (loadCatalog(*running, language)) {
        QCoreApplication::installTranslator(running);
    }
}

// Once in a run: registering a font again would give a second family entry.
void loadShinkiFonts()
{
#ifdef MITORI_SHINKI_FILES
    static bool loaded = false;
    if (loaded) {
        return;
    }
    loaded = true;
    QDirIterator fonts(QStringLiteral(":/shinki/fonts"), {QStringLiteral("*.ttf")});
    while (fonts.hasNext()) {
        QFontDatabase::addApplicationFont(fonts.next());
    }
#endif
}
} // namespace

Appearance::Appearance(QObject *parent)
    : QObject(parent)
    , m_style(saved())
    , m_running(Style(QSettings().value(QLatin1String(kRunningKey), int(saved())).toInt()))
    , m_language(savedLanguage())
    , m_welcomed(QSettings().value(QLatin1String(kWelcomedKey), false).toBool())
{
}

Appearance::~Appearance()
{
    delete m_window.data();
}

bool Appearance::offers(Style style, const QStringList &importPaths)
{
    // One controls style in a run: Plasma's where it is, else Material.
    if (style == Style::Breeze) {
        return hasBreeze(importPaths);
    }
    if (style == Style::Material) {
        return !hasBreeze(importPaths);
    }
#ifndef MITORI_SHINKI
    if (style == Style::Shinki) {
        return false;
    }
#endif
    return true;
}

Appearance::Style Appearance::startStyle(const QStringList &importPaths)
{
    const Style style = saved();
    const Style start = offers(style, importPaths) ? style : kDefaultStyle;
    QSettings().setValue(QLatin1String(kRunningKey), int(start));
    return start;
}

QString Appearance::controlsStyle(Style style, const QStringList &importPaths)
{
    switch (style) {
    case Style::Breeze:
        return QStringLiteral("org.kde.desktop");
    case Style::Material:
        return QStringLiteral("Material");
    case Style::Shinki:
        // Shinki's controls style comes with shinki/qml/Shinki; until then Kante's controls.
    case Style::Kante:
    case Style::Leinen:
        break;
    }
    return hasBreeze(importPaths) ? QStringLiteral("org.kde.desktop") : QStringLiteral("Material");
}

QString Appearance::localeName(Language language)
{
    switch (language) {
    case Language::German:
        return QStringLiteral("de");
    case Language::English:
        return QStringLiteral("en");
    case Language::System:
        break;
    }
    return {};
}

void Appearance::startLanguage()
{
    applyLanguage(savedLanguage());
}

QString Appearance::version() const
{
    return QStringLiteral(MITORI_VERSION);
}

QStringList Appearance::importPaths() const
{
    const QQmlEngine *engine = qmlEngine(this);
    return engine ? engine->importPathList() : QStringList();
}

QVariantList Appearance::styles() const
{
    const struct {
        Style style;
        const char *name;
        const char *text;
    } all[] = {
        {Style::Breeze, QT_TR_NOOP("Breeze"), QT_TR_NOOP("Plasma's own look, with its colours")},
        {Style::Material, QT_TR_NOOP("Material"), QT_TR_NOOP("Android's look, light")},
        {Style::Kante, QT_TR_NOOP("Kante"), QT_TR_NOOP("Dark, sharp edges, yellow accent")},
        {Style::Leinen, QT_TR_NOOP("Leinen"), QT_TR_NOOP("Kante in light linen")},
        {Style::Shinki, QT_TR_NOOP("Shinki"), QT_TR_NOOP("Paper, ink and one red, as the Shinki News")},
    };
    const QStringList paths = importPaths();
    QVariantList result;
    for (const auto &s : all) {
        if (!offers(s.style, paths)) {
            continue;
        }
        result << QVariantMap{{QStringLiteral("style"), int(s.style)},
                              {QStringLiteral("name"), tr(s.name)},
                              {QStringLiteral("text"), tr(s.text)}};
    }
    return result;
}

void Appearance::setStyle(Style style)
{
    if (style == m_style && style == m_running) {
        return;
    }
    m_style = style;
    QSettings().setValue(QLatin1String(kStyleKey), int(style));
    if (m_window && !m_rebuildAsked) {
        // After the tap: the tile that asks lives in the window that goes.
        m_rebuildAsked = true;
        QMetaObject::invokeMethod(this, &Appearance::rebuild, Qt::QueuedConnection);
    }
    if (!m_window) {
        m_running = style;
        QSettings().setValue(QLatin1String(kRunningKey), int(style));
    }
    emit changed();
}

void Appearance::setLanguage(Language language)
{
    if (language == m_language) {
        return;
    }
    m_language = language;
    QSettings().setValue(QLatin1String(kLanguageKey), int(language));
    applyLanguage(language);
    if (QQmlEngine *engine = qmlEngine(this)) {
        engine->retranslate();
    }
    emit languageChanged();
}

void Appearance::adopt(QObject *window)
{
    m_window = qobject_cast<QWindow *>(window);
    emit windowChanged();
}

void Appearance::setBarColor(const QColor &color)
{
    if (color == m_barColor) {
        return;
    }
    m_barColor = color;
    paintSystemBars(color);
    emit barColorChanged();
}

QVariantMap Appearance::sample(Language language) const
{
    // The sample's words, in Appearance's context of the catalogs: never data of the user.
    const struct {
        const char *key;
        const char *source;
    } words[] = {
        {"title", QT_TR_NOOP("Practice today")},
        {"text", QT_TR_NOOP("Tap a technique: its place in the exam video plays.")},
        {"chip", QT_TR_NOOP("exam-ready")},
        {"button", QT_TR_NOOP("Videos")},
    };
    QTranslator catalog;
    const bool loaded = loadCatalog(catalog, language);
    QVariantMap result;
    for (const auto &word : words) {
        const QString text = loaded ? catalog.translate("Appearance", word.source) : QString();
        result.insert(QLatin1String(word.key), text.isEmpty() ? QString::fromUtf8(word.source) : text);
    }
    return result;
}

QObject *Appearance::openWindow(QQmlEngine *engine, const QVariantMap &properties)
{
    // Only here: the component holds Main and all it builds on in the engine's cache.
    QQmlComponent main(engine);
    main.loadFromModule("Mitori", "Main");
    QObject *window = main.createWithInitialProperties(properties);
    if (!window) {
        qWarning().noquote() << main.errorString();
        return nullptr;
    }
    return window;
}

// The window anew in the chosen style:
//
//   keptPageList()   the pages to come back to; text fields left (they save on leaving)
//   delete window    pages go, a note being typed is saved (NoteRow), a video stops
//   prepare          the file selector: LookButton is now the other file...
//   clear cache      ...once the compiled one has left the engine's cache
//   Look.reload      the roles of the style (the singleton itself and the services stay)
//   openWindow       with the kept pages and the old size
void Appearance::rebuild()
{
    m_rebuildAsked = false;
    QQmlEngine *engine = qmlEngine(this);
    if (!m_window || !engine) {
        return;
    }

    QVariant kept;
    QMetaObject::invokeMethod(m_window.data(), "keptPageList", Q_RETURN_ARG(QVariant, kept));
    const QVariantMap properties{{QStringLiteral("width"), m_window->width()},
                                 {QStringLiteral("height"), m_window->height()},
                                 {QStringLiteral("keptPages"), kept}};
    const QWindow::Visibility visibility = m_window->visibility();

    // Between the windows there is none for a moment: that is no reason to quit.
    const bool quitOnClose = QGuiApplication::quitOnLastWindowClosed();
    QGuiApplication::setQuitOnLastWindowClosed(false);
    delete m_window.data();
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    engine->collectGarbage();
    engine->clearComponentCache();

    m_running = m_style;
    QSettings().setValue(QLatin1String(kRunningKey), int(m_running));
    prepare(m_running, engine);
    QMetaObject::invokeMethod(engine->singletonInstance<QObject *>("Mitori", "Look"), "reload");
    emit changed();

    m_window = qobject_cast<QWindow *>(openWindow(engine, properties));
    QGuiApplication::setQuitOnLastWindowClosed(quitOnClose);
    if (!m_window) {
        QCoreApplication::exit(1);
        return;
    }
    if (visibility != QWindow::Hidden) {
        m_window->setVisibility(visibility);
    }
    emit windowChanged();
}

void Appearance::welcome()
{
    m_welcomed = true;
    QSettings().setValue(QLatin1String(kWelcomedKey), true);
    emit changed();
}

void Appearance::prepare(Style style, QQmlEngine *engine)
{
    if (style == Style::Shinki) {
        loadShinkiFonts();
    }
#ifdef MITORI_SHINKI_FILES
    auto *selector = engine->findChild<QQmlFileSelector *>(QString(), Qt::FindDirectChildrenOnly);
    if (!selector && style != Style::Shinki) {
        return;
    }
    if (!selector) {
        selector = new QQmlFileSelector(engine, engine);
    }
    selector->setExtraSelectors(style == Style::Shinki ? QStringList{QStringLiteral("shinki")} : QStringList());
#else
    Q_UNUSED(engine)
#endif
}
