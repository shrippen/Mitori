#pragma once

#include <QObject>
#include <QStringList>
#include <QVariantList>
#include <QtQml/qqmlregistration.h>

class QQmlEngine;

// How Mitori looks and speaks: the style (shinki/README.md, section 6), the language and whether
// the first-start wizard ran. Style and language apply at the next start: Qt sets the controls
// style and the catalog before the first window (main.cpp reads them with `startStyle` and
// `startLocale`).
//
//   style      controls (QQC2)        KanteStyle
//   Breeze     org.kde.desktop        System    (only where qqc2-desktop-style is, on Plasma)
//   Material   Material               System
//   Kante      Breeze or Material     Kante, dark
//   Leinen     Breeze or Material     Kante, light ("Leinen")
//   Shinki     Breeze or Material     Kante, light   (only with -DMITORI_SHINKI=ON; its own
//                                               controls and look come with app/qml/Look)
class Appearance : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    // The style chosen now (applies at the next start) and the one the app runs with.
    Q_PROPERTY(Style style READ style NOTIFY changed)
    Q_PROPERTY(Style running READ running CONSTANT)
    Q_PROPERTY(Language language READ language NOTIFY changed)
    // A choice differs from what runs: "applies at the next start".
    Q_PROPERTY(bool restartNeeded READ restartNeeded NOTIFY changed)
    Q_PROPERTY(bool welcomed READ welcomed NOTIFY changed)
    Q_PROPERTY(QString version READ version CONSTANT)
    Q_PROPERTY(QString keychainLicence READ keychainLicence CONSTANT)

public:
    enum class Style { Breeze, Material, Kante, Leinen, Shinki };
    Q_ENUM(Style)
    enum class Language { System, German, English };
    Q_ENUM(Language)

    explicit Appearance(QObject *parent = nullptr);

    Style style() const { return m_style; }
    Style running() const { return m_running; }
    Language language() const { return m_language; }
    bool restartNeeded() const;
    bool welcomed() const { return m_welcomed; }
    QString version() const;
    QString keychainLicence() const;

    // The styles this device offers, in order: [{style, name, text}].
    Q_INVOKABLE QVariantList styles() const;
    Q_INVOKABLE void setStyle(Style style);
    Q_INVOKABLE void setLanguage(Language language);
    // The wizard is done (or skipped): it does not come again.
    Q_INVOKABLE void welcome();

    // For main.cpp, before the first window: the saved style where the device has it, else the
    // default; its controls style; the language's locale ("de", "en"; empty: the user's).
    static Style startStyle(const QStringList &importPaths);
    static QString controlsStyle(Style style, const QStringList &importPaths);
    static QString startLocale();
    // Before the first window: Shinki's fonts and the file selector "shinki", so that
    // app/qml/Look answers with Shinki (its +shinki files); nothing for the other styles.
    static void prepare(Style style, QQmlEngine *engine);

signals:
    void changed();

private:
    Style m_style;
    Style m_running;
    Language m_language;
    Language m_startedLanguage;     // what main.cpp started with (restartNeeded)
    bool m_welcomed;

    QStringList importPaths() const;
    static bool offers(Style style, const QStringList &importPaths);
};
