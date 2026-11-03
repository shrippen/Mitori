#include "appearance.h"

#include <QDir>
#include <QLocale>
#include <QQmlEngine>
#include <QSettings>

namespace {
constexpr auto kStyleKey = "look/style";
constexpr auto kRunningKey = "look/running";     // what main.cpp started with (for the pages)
constexpr auto kLanguageKey = "look/language";
constexpr auto kStartedLanguageKey = "look/startedLanguage";   // what main.cpp started with
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
} // namespace

Appearance::Appearance(QObject *parent)
    : QObject(parent)
    , m_style(saved())
    , m_running(Style(QSettings().value(QLatin1String(kRunningKey), int(saved())).toInt()))
    , m_language(Language(QSettings().value(QLatin1String(kLanguageKey), int(Language::System)).toInt()))
    , m_welcomed(QSettings().value(QLatin1String(kWelcomedKey), false).toBool())
{
    m_startedLanguage = Language(QSettings().value(QLatin1String(kStartedLanguageKey), int(m_language)).toInt());
}

bool Appearance::offers(Style style, const QStringList &importPaths)
{
    if (style == Style::Breeze) {
        return hasBreeze(importPaths);
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

QString Appearance::startLocale()
{
    const Language language = Language(QSettings().value(QLatin1String(kLanguageKey), int(Language::System)).toInt());
    QSettings().setValue(QLatin1String(kStartedLanguageKey), int(language));
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

bool Appearance::restartNeeded() const
{
    return m_style != m_running || m_language != m_startedLanguage;
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
    m_style = style;
    QSettings().setValue(QLatin1String(kStyleKey), int(style));
    emit changed();
}

void Appearance::setLanguage(Language language)
{
    m_language = language;
    QSettings().setValue(QLatin1String(kLanguageKey), int(language));
    emit changed();
}

void Appearance::welcome()
{
    m_welcomed = true;
    QSettings().setValue(QLatin1String(kWelcomedKey), true);
    emit changed();
}
